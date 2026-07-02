# Inter-Kernel Pipes

This example demonstrates how to use inter-kernel pipes to transfer data directly between two concurrently executing kernels on the FPGA, without involving the host or external memory. It shows both blocking and non-blocking pipe operations, illustrating how blocking pipes provide guaranteed data delivery with backpressure while non-blocking pipes handle data that arrives intermittently.

## Inter-Kernel Pipe Overview

Inter-kernel pipes are on-device FIFO buffers that connect two kernels running concurrently. Data written by one kernel is available for reading by the other with low latency and no external memory access. Typical use cases include:

- Streaming data from a producer stage to a consumer stage in a processing pipeline
- Sending control signals or notifications between cooperating kernels
- Decoupling kernels with different processing rates via buffering

Inter-kernel pipes are single-directional. Bidirectional communication between kernels can only be achieved by using two pipes.

### Declaration

Each individual inter-kernel pipe is a program-scope specialization of the templated pipe class. Example declaration:

```cpp
using InterKernelPipe = sycl::ext::altera::experimental::pipe<
    class PipeID,        // Pipe identifier (a unique class)
    int,                // Data type
    8>;                 // Minimum pipe capacity
```

The first template parameter should be a unique user-defined type that serves as the pipe's identifier. The second template parameter defines the data type of each element carried by the pipe.

The third template parameter is optional and defines the minimum pipe capacity (defaults to 0), which is the guaranteed minimum number of elements that can be held in the pipe without anything being read out. Specify this parameter to prevent potential deadlock.

> **Note**: The compiler may increase the actual FIFO capacity when it determines a larger buffer is beneficial.

Unlike host pipes, pipe properties (such as `protocol` and `uses_valid`) may not be specified for inter-kernel pipes. See the [Host Pipes](../host_pipes) and [CSR Pipes](../csr_pipes) examples for details on pipe properties.

### Read/Write API

Inter-kernel pipes support both blocking and non-blocking read/write operations:

| | Blocking | Non-Blocking |
|---|---|---|
| **Read** | `auto d = Pipe::read();` | `bool success = false;`<br>`auto d = Pipe::read(success);` |
| **Write** | `Pipe::write(d);` | `bool success = false;`<br>`Pipe::write(d, success);` |

**Blocking** operations provide natural backpressure -- a write blocks when the pipe is full and a read blocks when it is empty, so every element written is guaranteed to be read. This enables lockstep synchronization between the source and sink kernels.

**Non-blocking** operations return immediately regardless of pipe state. The `success` flag indicates whether the operation completed. These are useful when auxiliary data arrives at unpredictable intervals and the kernel must continue processing without stalling.

Blocking and non-blocking calls can be mixed on a single pipe. For example, the writer may use blocking writes while the reader uses non-blocking reads. Multiple reads or writes to the same pipe are allowed, but each endpoint of the pipe must reside in exactly one kernel.

## Design Overview

This design demonstrates both blocking and non-blocking inter-kernel pipe reads between a `Producer` and a `Consumer` kernel. It is assumed that the `Consumer` knows the data will arrive every loop iteration through the `DataPipe`, but it does not know when a signal will arrive through the `SignalPipe`. Therefore, the pipes are used in the following ways:

- `DataPipe`: carries the main data elements. The `Producer` uses blocking writes and the `Consumer` uses blocking reads, ensuring every element is delivered and the computation results are verifiable.
- `SignalPipe`: represents intermittent data that the `Consumer` cannot predict the arrival of. The `Producer` writes a signal every 16th element (using blocking writes), and the `Consumer` uses non-blocking reads on every iteration to check if a signal is available.

Note that in this design, the blocking `DataPipe` read/write acts as a synchronization point between the two kernels: the `Producer` writes the signal *before* the corresponding data element so that the signal is already visible in `SignalPipe` by the time the `Consumer`'s blocking `DataPipe` read returns. The `Consumer` cannot advance past a `DataPipe` read until the `Producer` has written that element. In this way, we ensure that all the signal writes will be captured despite a non-blocking read being used. The `Consumer` counts how many signals it received and reports the total to the host via an external memory buffer. The host verifies that all signals were eventually delivered (total equals `kArraySize/16`) and that all data elements were processed correctly.

## Example Output

```
Processing <kArraySize> elements
Signals received: <kArraySize/16> / <kArraySize/16>
PASSED
```

The signal count should always equal `kArraySize/16`, confirming that all signals were eventually delivered despite the intermittent arrival pattern.

## Related Samples

- For streaming host pipes that connect the host to a kernel, see the [Host Pipes](../host_pipes) example.
- For CSR-mapped pipes used as control signals, see the [CSR Pipes](../csr_pipes) example.
