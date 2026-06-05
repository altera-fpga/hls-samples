# CSR-Mapped Host Pipes

This example demonstrates how to use CSR (Control/Status Register)-mapped host pipes (also known as CSR pipes) to send control signals to a long running kernel while it is processing streaming data. It also shows how to combine non-blocking streaming pipe reads with non-blocking CSR pipe reads to establish a stop signaling mechanism that can interrupt the kernel at any time without stalling the data processing loop.

## CSR Pipe Overview

CSR pipes are host pipes that use the Avalon® memory-mapped protocol instead of the default Avalon streaming protocol. Each CSR pipe maps to a single hardware register, making them ideal for **low-bandwidth control signals** rather than bulk data transfer. Typical use cases include:

- Gracefully stopping a long running kernel loop
- Publishing status counters or error flags that the external system can poll
- Changing runtime parameters such as thresholds or filter coefficients while the kernel is running

### Declaration

Each individual CSR pipe is a program-scope specialization of the templated pipe class, with the `avalon_mm` protocol property:

```cpp
using CsrProperties = decltype(sycl::ext::oneapi::experimental::properties(
    sycl::ext::altera::experimental::protocol_avalon_mm));

using CsrPipe = sycl::ext::altera::experimental::pipe<
    class CsrID,      // Pipe identifier (a unique class)
    int,              // Data type
    0,                // Minimum pipe capacity
    CsrProperties>;   // Pipe properties (set to CSR)
```

The first template parameter should be a unique user-defined type that serves as the pipe's identifier. The second template parameter defines the data type of each element carried by the pipe.

Since CSR pipes are register-mapped, the third template parameter, `min_capacity`, is ignored (set to 0 in the above example).

The following streaming-specific properties do not apply to CSR pipes and must not be specified: 

- `bits_per_symbol`
- `first_symbol_in_high_order_bits`
- `ready_latency`.

Only `protocol`, `uses_valid`, and `uses_ready` are relevant for CSR pipes.

### Read/Write API

CSR pipes use the same overloaded `pipe::read()` and `pipe::write()` static methods as streaming pipes on both the kernel side and the host side. Host-side calls take an additional `sycl::queue` argument to identify the device.

The following table summarizes the API:

<table>
  <tr>
    <th colspan="2"></th>
    <th>Blocking</th>
    <th>Non-Blocking</th>
  </tr>
  <tr>
    <td rowspan="2"><b>Read</b></td>
    <td><b>Kernel</b></td>
    <td><code>auto d = Pipe::read();</code></td>
    <td><code>bool success = false;</code><br><code>auto d = Pipe::read(success);</code></td>
  </tr>
  <tr>
    <td><b>Host</b></td>
    <td><code>auto d = Pipe::read(q);</code></td>
    <td><code>bool success = false;</code><br><code>auto d = Pipe::read(q, success);</code></td>
  </tr>
  <tr>
    <td rowspan="2"><b>Write</b></td>
    <td><b>Kernel</b></td>
    <td><code>Pipe::write(d);</code></td>
    <td><code>bool success = false;</code><br><code>Pipe::write(d, success);</code></td>
  </tr>
  <tr>
    <td><b>Host</b></td>
    <td><code>Pipe::write(q, d);</code></td>
    <td><code>bool success = false;</code><br><code>Pipe::write(q, d, success);</code></td>
  </tr>
</table>

Blocking and non-blocking API can be mixed on a single pipe (i.e., one end performs blocking operation while the other end performs non-blocking operation).

Non-blocking reads are particularly important for CSR pipes used as control signals: if the kernel used a blocking read, it would stall the entire processing loop until the external system writes a value. With non-blocking reads the kernel can check for control updates on every iteration without ever stalling.

### Stalling

A **stalling** CSR pipe has handshaking signals (valid/ready) that prevent the writer from producing a new value until the reader has consumed the previous one. A **stall-free** CSR pipe omits these handshaking signals: the writer can update the register at any time, and the reader always sees whatever value is currently in the register, even if it has already been read or was never updated.

The `uses_valid` and `uses_ready` properties control which mode a CSR pipe uses, and this interacts with the blocking/non-blocking API:

For an **input** CSR pipe (external system writes, kernel reads):

- **Stalling** (`uses_valid<true>`): the pipe has both a data register and a valid register.
  - **Blocking**: External system waits until the kernel has consumed the previous value (valid == 0) before writing new data.
  - **Non-blocking**: External system write checks the valid register and returns failure if the kernel hasn't consumed the previous value yet.
- **Stall-free** (`uses_valid<false>`): the pipe has only a data register.
  - **Blocking** and **non-blocking**: The external system always overwrites the data regardless of whether the kernel has consumed it.

For an **output** CSR pipe (kernel writes, external system reads):

- **Stalling** (`uses_ready<true>`): the pipe has both a data register and a ready register.
  - **Blocking**: External system waits until the kernel has produced new data (ready == 0) before reading.
  - **Non-blocking**: External system read checks the ready register and returns failure if no new data is available.
- **Stall-free** (`uses_ready<false>`): the pipe has only a data register.
  - **Blocking** and **non-blocking**: The external system always reads whatever is in the register.

Configure these properties in the pipe's properties object:

```cpp
// Stalling input CSR: external system can confirm kernel consumed the value
using CsrInProperties = decltype(sycl::ext::oneapi::experimental::properties(
    sycl::ext::altera::experimental::protocol_avalon_mm,
    sycl::ext::altera::experimental::uses_valid_on));

// Stall-free output CSR: kernel updates freely, external system polls at its own pace
using CsrOutProperties = decltype(sycl::ext::oneapi::experimental::properties(
    sycl::ext::altera::experimental::protocol_avalon_mm,
    sycl::ext::altera::experimental::uses_ready_off));
```

> **Note**: `uses_ready` cannot be set for input pipes and `uses_valid` cannot be set for output pipes. The stall-free/stalling behavior of an input pipe is controlled solely by `uses_valid`, and for an output pipe solely by `uses_ready`.

### Comparison with Streaming Pipes

| Property | Streaming Pipe | CSR Pipe |
|---|---|---|
| Protocol | Avalon ST / AXI ST | Avalon MM |
| Buffer depth | Configurable (`min_capacity`) | Fixed at 1 (register) |
| Best for | High-throughput data transfer | Low-bandwidth control signals |
| Pre-fill before kernel? | Yes, recommended for simulation | Not applicable (register-based) |
| Throughput tracking | Yes | No |

> **Warning**: CSR pipes must only be used with a register-mapped invocation interface. Using them with a streaming invocation interface yields undefined behavior.

## Design Overview

In this example, `PassthroughKernel` runs a long running processing loop. On each iteration it performs non-blocking reads from the streaming `InputPipe` for data and from `StopCSR` for a stop signal issued by the external system. When data is available, the kernel forwards it to `OutputPipe` and updates `CountCSR` with the running total of processed elements. When the external system writes `true` to `StopCSR`, the kernel picks it up on the next iteration and exits the loop.

`StopCSR` is configured as **stalling** (`CsrInProperties` sets `uses_valid_on`) so the external system can confirm the kernel has consumed the stop command before proceeding. `CountCSR` is configured as **stall-free** (`CsrOutProperties` sets `uses_ready_off`) so the kernel can update the count on every iteration without waiting for the external system to acknowledge each update -- the external system simply polls at its own pace.

## Example Output

```
Processing 256 elements
Kernel processed 256 elements
PASSED
```

## Related Samples

- For a real-world design that uses CSR pipes for stop/bypass control, see the [Convolution 2D](/ReferenceDesigns/convolution2d) reference design.
- For CSR-mapped pipes used as a data interface (passing vector data element by element), see the [CSR Pipes](/Tutorials/Features/hls_flow_interfaces/component_interfaces_comparison/csr-pipes) example in the Component Interfaces Comparison sample.
