# ring_buffer

## Introduction 
This project aims to implement a mock ring buffer using C multithreading. The implementation will subsequently be ported to an FPGA to support the efficient execution of "Configurable DSP-Based CAM Architecture for Data-Intensive Applications on FPGAs". 

In this project, we focus on a one-to-one ring buffer implementation. This model is ideal for our target FPGA platform, which requires high-speed communication between the Processing System (PS) and Programmable Logic (PL).

The original implementation sends instructions from the PS to the DSP-based CAM in the PL through a General Purpose (GP) port using AXI4-Lite. AXI4-Lite is register-based: every register write is a separate, single-beat AXI transaction with its own address, data and response handshake, and bursts are not supported. One CAM instruction (an 8-bit header plus a 512-bit payload) therefore takes many transactions, and the instruction channel becomes the bottleneck of the whole system.

This project replaces that channel with memory-based communication: a ring buffer placed in On-Chip Memory (OCM). The PS writes instructions into the buffer and advances `head`; the PL reads them through a High Performance (HP) AXI port and advances `tail`. The channel still uses AXI. The saving comes from how AXI is used, as explained below.

Target board: ALINX AX7020 (Zynq-7020).

### Status
- Done: the C prototype of the ring buffer core and its test suite (this repository).
- Not done yet: porting to the FPGA and on-board measurement. The benefits listed below are design expectations, not measured results.

### Technical Deep Dive: AXI4-Lite Registers vs. OCM Ring Buffer
#### Where the overhead comes from
- **One transaction per register.** With AXI4-Lite the PS can only write one register at a time. Each write goes through the full write-address, write-data and write-response handshakes.
- **No burst.** AXI4-Lite has no burst transfers, so a wide instruction cannot be sent as one transaction.
- **Shared path.** The GP port is reached through the PS Central Interconnect, which is shared with other PS traffic.

#### Why the OCM ring buffer is expected to be faster
1. **Far fewer AXI transactions.** Communication becomes memory-based. The PL fetches instructions from OCM in bursts, so one transaction moves many data beats (and several instructions) instead of one register.
2. **HP port path.** The HP port goes through the PL to Memory Interconnect instead of the Central Interconnect, which gives higher bandwidth and less contention.
3. **Native burst support.** The HP ports support burst access, which the register interface cannot use.
4. **Decoupling.** Instead of waiting for a response to every register write, the PS writes an instruction into the buffer and moves on. The PL consumes instructions at its own pace.


## What is Ring Buffer?
A ring buffer (or circular buffer) is a fundamental data transfer mechanism, particularly useful for asynchronous processes where a producer and consumer operate at different speeds.

### Key Characteristics
- **Fixed Size:** Memory is pre-allocated, avoiding dynamic allocation overhead during runtime.
- **Lock-Free Potential:** In a Single-Producer Single-Consumer (SPSC) model, the ring buffer can be implemented without expensive mutexes or semaphores, provided that pointer updates are atomic and memory barriers are respected.
- **FIFO Logic:** Data is processed in the order it was received.

A typical ring buffer is managed by `head` and `tail` pointers:
1. **Producer (PS):** Writes data to the `head` and then increments it.
2. **Consumer (PL):** Reads data from the `tail` and then increments it.

The buffer is "Full" when `(head + 1) % SIZE == tail` and "Empty" when `head == tail`.


## Implementation Details
### Language Selection
While C++ is increasingly common in embedded systems development, we have chosen C for the following reasons:
1. **Toolchain Compatibility:** The PS-side control for our FPGA (Zynq®-7000) is natively supported by C-based Xilinx drivers.
2. **Memory Determinism:** C structs guarantee a predictable memory layout. By using `__attribute__((packed))` and specific alignment pragmas, we can ensure the software structure maps exactly to the hardware-defined OCM addresses. C++ features like virtual tables or name mangling can introduce hidden offsets.
3. **Execution Predictability:** C avoids implicit overheads (e.g., hidden constructors or exception handling logic). This is critical for meeting the strict timing requirements of PL-PS synchronization.

### Technical Specifications
- **Concurrency:** Single-Producer Single-Consumer (SPSC).
- **Synchronization:** The mock implementation uses atomic operations (`stdatomic.h`) to simulate hardware memory consistency. In the final FPGA port, explicit memory barriers (DMB/DSB instructions) will be used to ensure the PL sees the data write before the head pointer update.
- **Cache Coherence:** Shared OCM ring-buffer memory is treated as **non-cacheable** on the PS side for the first implementation stage. This matches the intended PL direct memory access behavior and avoids stale cache-line visibility issues during bring-up.
- **Ring Buffer Size:** 16 KB (16,384 Bytes)
- **Instruction Unit Size:** 512 bits (64 Bytes)
- **Overflow Policy:** "Drop Latest" – if the buffer is full, the producer will not overwrite unread data, ensuring data integrity for the CAM hardware.

### Future Testing Note: AXI Performance & Instruction Width
During on-board testing on the FPGA, we may evaluate increasing the **Instruction Unit Size** to **544 bits**. 
- **The Rationale:** There is a potential concern that exceeding the standard **512-bit** boundary might trigger additional **AXI Packaging/Re-alignment** overhead in the interconnect, which could impact throughput or latency. 
- **Goal:** We will benchmark both **512-bit** and **544-bit** configurations to verify if AXI re-packing occurs and choose the size that yields the optimal system performance.

### Optimization
Using a power-of-two size (**16 KB**) allows us to replace the costly modulo operator (`%`) with a bitwise AND (`&`). With a **64-Byte** instruction, this buffer holds exactly **256 instructions**, preventing any wrap-around splitting at the boundaries.
```c
// Instead of:
head = (head + 1) % SIZE;
// We use:
head = (head + 1) & (SIZE - 1);
```

## Current Test Workflow
The current test suite focuses on fast, deterministic functional validation of the SPSC ring buffer.

### How to Run
- Build all test binaries:
  - `make all`
- Run all tests:
  - `make test`
- Run a specific test:
  - `make run-boundary`
  - `make run-wrap-drop`
  - `make run-integration`

### What Each Test Covers
- `boundary_test`
  - Validates empty-buffer consume behavior, full-buffer write rejection, FIFO drain correctness, and post-drain empty behavior.
- `wrap_drop_test`
  - Validates wrap-around index correctness across ring boundaries and confirms drop-latest does not overwrite unread data.
- `integration_test`
  - Runs threaded producer/consumer behavior for a short duration (default 2 seconds) and validates end-to-end ordering/integrity counters (`Produced`, `Consumed`, `Dropped`, `Errors`).