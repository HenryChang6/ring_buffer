## 2026-02-11
### Technical Insights: Why Mutexes and Semaphores are "Expensive"

During the refinement of the ring buffer documentation, we discussed why traditional synchronization primitives like Mutexes and Semaphores are considered costly in high-performance and embedded contexts (especially for PS-PL communication).

#### 1. The Cost of Context Switching
When a thread fails to acquire a Mutex, the OS often puts it to sleep.
- **State Saving:** The CPU must save all registers and stack pointers for the current thread.
- **Cache Pollution:** Switching to a new thread flushes the pipeline and brings in new data, causing "cold misses" when the original thread resumes. This is catastrophic for timing-critical FPGA data streams.

#### 2. Kernel/User Space Transitions
Mutexes are managed by the OS kernel.
- **System Calls:** Requesting a lock involves a transition from User Mode to Kernel Mode. This context switch can take hundreds or thousands of clock cycles, which is unacceptable for high-frequency ring buffer operations.

#### 3. Bus Locking and Memory Barriers
Even spinlocks (which don't sleep) require atomic instructions (e.g., `CAS`, `LDREX/STREX`).
- **Cache Line Bouncing:** The memory address of the lock bounces between CPU cores, creating heavy traffic on the internal cache coherency bus.
- **Memory Barriers:** CPU pipelines must be stalled to ensure all previous memory writes are visible to other cores before the lock is released.

#### 4. Non-Determinism in Real-Time Systems
- **Priority Inversion:** Semaphores can lead to scenarios where a high-priority task is blocked by a low-priority one.
- **Unpredictable Latency:** You cannot guarantee how long a lock will be held, which introduces "jitter" in hardware-to-software communication.

#### 5. The Lock-Free Advantage for SPSC
Our **Single-Producer Single-Consumer (SPSC)** ring buffer avoids these costs entirely:
- **Ownership Split:** The Producer *only* writes to the `head` pointer; the Consumer *only* writes to the `tail` pointer.
- **Atomic Progress:** By using atomic stores/loads (and memory barriers in hardware), we ensure that as long as the buffer isn't full/empty, both sides can progress simultaneously without ever needing to "lock" the other.
- **Efficiency:** This maps directly to hardware logic, enabling the high throughput required for the "Configurable DSP-Based CAM Architecture."

### Project Specification Update
- **Instruction Size:** Defined as **512 bits (64 Bytes)**. This aligns with standard AXI data widths and provides a clean mapping for the CAM's Opcode/Address/Data.
- **Buffer Capacity:** Increased to **16 KB**. This allows for **256 instructions**, providing enough "cushion" to absorb PS-side OS jitter.
- **AXI Benchmarking:** Added a task to test **544-bit** instruction widths on-board to investigate potential AXI re-packing performance penalties.

## 2026-02-12
Discussion with Feng:
> 我們目前 CAM 的 DATA_WIDTH 是 520 bits，是 8 bits 的 header 加上 512 bits 的 payload（剛好 16 個 32-bit value），這個 DATA_WIDTH 當初為什麼會這樣設計？我疑惑的點是因為它既不能被 32 整除也不能被 64 整除。目前我在用 C Mock up Ring Buffer 時在想一個指令的長度要設多少，發現 520 這個數字比較尷尬，因為 Zynq 的 OCM 要透過 AXI-64 讀取，如果 data 是 520 bits 的話就會是沒有對齊的狀態，這樣傳一次 data 可能會觸發兩次 memory 讀取，導致 Latancy 增加。

Conclusion:
> 對，會有 Alignment 的問題。當初 520 bits 是從 accelerator 那邊過來的，是可以調整的。要注意的是改成 512 bits 和 544 bits 都可以，但上板子測試的時候要注意因為 AXI 協議那邊超過 512 bits 的好像會再打包一次，造成不必要的 overhead，到時候要再測試一下。

## 2026-02-14
Memory Order
1. relaxed
- 只保證 atomic 變數本身操作是 atomic 的，不提供跨變數的先後順序保證
- 用途：讀自己擁有的 index（e.g., producer 讀 head、consumer 讀 tail）
2. release
- 用在 store action，保證在這個 release store 之前的寫入，不會被重排到它後面
3. acquire
- 用在 load action，保證在這個 acquire load 之後的讀寫，不能被重排到它前面

在 producer thread 當中，所有 release store 前的指令，會在 consumer threaad 進行 acquire load