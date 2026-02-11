# ring_buffer

## Introduction 
This project aims to implement a mock ring buffer using C multithreading. The implementation will subsequently be ported to an FPGA to support the efficient execution of "Configurable DSP-Based CAM Architecture for Data-Intensive Applications on FPGAs". 

In this project, we focus on a one-to-one ring buffer implementation. This model is ideal for our target FPGA platform, which requires high-speed communication between the Processing System (PS) and Programmable Logic (PL).

The original implementation used General Purpose (GP) AXI-Lite ports to send instructions from the PS to the DSP-based CAM hardware in the PL. However, the AXI-Lite protocol introduces significant overhead due to bus arbitration and handshake latencies, becoming a system bottleneck. By implementing a ring buffer in On-Chip Memory (OCM), we leverage dual-port RAM characteristics, allowing the PS and PL to access shared data with minimal latency and significantly higher throughput compared to standard bus transactions.

### Technical Deep Dive: AXI-Lite Bottleneck vs. OCM Efficiency
#### What is AXI Bus Arbitration?
In a SoC like the Zynq-7000, multiple "Master" devices (e.g., CPU cores, DMA, Video Engines) share the same "Slave" resources via an Interconnect. **AXI Bus Arbitration** is the management mechanism that decides which Master gets control of the bus when multiple requests occur simultaneously. 

Think of it as a traffic light:
- **Latencies:** Every transaction must request access, wait for the arbiter to grant it, and perform a multi-step handshake (`VALID`/`READY`).
- **Jitter:** If other parts of the system are busy, the CPU might wait longer to send a single command, causing unpredictable timing.

#### Why OCM is Faster
By moving to an **OCM-based Ring Buffer**, we effectively bypass the "traffic light":
1. **Dedicated Path:** OCM in Zynq is designed for low-latency, high-priority access. It often features multiple ports, allowing the PS and PL to access memory simultaneously without competing for the same bus cycle.
2. **Asynchronous Decoupling:** Instead of a "Stop-and-Wait" approach (where the CPU waits for an AXI response), the PS simply writes data to the buffer and moves on. The PL reads the data at its own clock rate.
3. **Reduced Overhead:** We eliminate the per-transaction arbitration overhead, replacing many small AXI-Lite bursts with a continuous stream of data through shared memory.    


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
- **Synchronization:** The mock implementation uses atomic operations (`stdatomic.h`) to simulate the hardware memory consistency. In the final FPGA port, explicit memory barriers (DMB/DSB instructions) will be used to ensure the PL sees the data write before the head pointer update.
- **Buffer Size:** 1024 bytes (Power of 2).
- **Optimization:** Using a power-of-two size allows us to replace the costly modulo operator (`%`) with a bitwise AND (`&`).
  ```c
  // Instead of:
  head = (head + 1) % SIZE;
  // We use:
  head = (head + 1) & (SIZE - 1);
  ```
- **Overflow Policy:** "Drop Latest" – if the buffer is full, the producer will not overwrite unread data, ensuring data integrity for the CAM hardware.