# Streaming Host Pipes

This example demonstrates streaming host pipes, which transfer data between a kernel in the HLS IP Gen compiler-generated IP core and an external system such as a host CPU, an Ethernet interface, or another IP core. It covers blocking and non-blocking operations, and the recommended simulation pattern of pre-filling input data before launching the kernel.

## Streaming Host Pipe Overview

Streaming host pipes are FIFO-based streaming interfaces that connect an external system to a kernel. Unlike inter-kernel pipes, which have both endpoints inside the kernel system, a host pipe has only one such endpoint. The compiler infers the pipe direction and generates a corresponding streaming interface on the IP core boundary. Typical use cases include:

- Streaming input data from an external system to a kernel for processing
- Streaming processed results from a kernel back to the external system

Host pipes are single-directional. Bidirectional communication between an external system and an IP core requires two separate pipes (one for each direction).

### Declaration

Each individual streaming host pipe is a program-scope specialization of the templated pipe class. Example declaration:

```cpp
using DefaultProperties = decltype(sycl::ext::oneapi::experimental::properties(
    sycl::ext::altera::experimental::protocol_avalon_streaming,
    sycl::ext::altera::experimental::bits_per_symbol<8>,
    sycl::ext::altera::experimental::first_symbol_in_high_order_bits<false>,
    sycl::ext::altera::experimental::uses_valid_on,
    sycl::ext::altera::experimental::uses_ready_on,
    sycl::ext::altera::experimental::ready_latency<0>));

using StreamPipe = sycl::ext::altera::experimental::pipe<
    class PipeID,        // Pipe identifier (a unique class)
    int,                 // Data type
    8,                   // Minimum pipe capacity
    DefaultProperties>;  // Pipe properties
```

The first template parameter should be a unique user-defined type that serves as the pipe's identifier. The second template parameter defines the data type of each element carried by the pipe.

The third template parameter is optional and defines the minimum pipe capacity (defaults to 0), which is the guaranteed minimum number of elements that can be held in the pipe without anything being read out. Specify this parameter to prevent potential deadlock.

> **Note**: The compiler may increase the actual FIFO capacity when it determines a larger buffer is beneficial.

Compared to inter-kernel pipes, host pipes can take a fourth optional `properties` template parameter to configure the streaming interface. The detailed usage of these properties is beyond the scope of this tutorial. You can find their definitions and usage in the *[HLS IP Gen Handbook](https://docs.altera.com/r/docs/615048/current)*. The above shows all properties set to their default values. Omitting the `properties` parameter entirely produces the same result.

Among all the properties, `protocol` is a special one that allows you to configure the pipe to be either streaming (Avalon® ST via `protocol_avalon_streaming`, or AXI ST via `protocol_axi_streaming`) or register-mapped (Avalon MM via `protocol_avalon_mm`). See the [CSR Pipes](../csr_pipes) example for details on register-mapped host pipes.

### Read/Write API

Host pipes use the overloaded `pipe::read()` and `pipe::write()` static methods on both the kernel side and the host side. The host code serves as a testbench that drives data into the kernel and collects results. Host-side calls take an additional `sycl::queue` argument to identify the device.

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

Blocking operations wait until the operation can complete (read waits for data, write waits for space in the FIFO). Non-blocking operations return immediately and set the `bool&` flag to indicate whether the operation succeeded.

Blocking and non-blocking calls can be mixed on a single pipe. For example, the writer may use blocking writes while the reader uses non-blocking reads. Multiple reads or writes to the same pipe are allowed, but the kernel side endpoint of the pipe must reside in exactly one kernel.

### Simulation Pattern: Pre-Fill Before Kernel Launch

When simulating a kernel system with input streaming host pipes, write all input data to the host pipe **before** launching the kernel. This ensures the host can supply new data to the kernel every clock cycle once execution begins, giving the most accurate simulation performance estimate:

```cpp
// Pre-fill all input data
for (size_t i = 0; i < count; i++)
  InputPipe::write(q, in[i]);

// Launch kernel -- it begins consuming data immediately
auto e = SubmitKernel(q, count);
```

To check the streaming data throughput in a pipe, open the `report.html` file in the `pipes.fpga_sim.prj/reports/` directory after compiling the `fpga_sim` target and running the executable. Under the *Simulation Statistics* panel in the *Throughput Analysis* menu, pipe throughput can be checked in the *External Pipe Metrics* section. Clicking on one of these rows gives further information about the selected pipe in the details pane. As this example employs this design pattern, you should see a *Cycles/Transaction* value of `1` for all three pipes.

<p align="center">
  <img src=../assets/stream_pipe_report.png />
</p>

> **Note**: When using this pattern, `min_capacity` must be large enough to generate a FIFO capable of holding all pre-filled elements. If the pipe fills up before the kernel is launched, the host will block causing deadlock.

## Design Overview

This design demonstrates both blocking and non-blocking host pipe operations, on pipes with a default Avalon streaming protocol. For a more advanced host pipe example with different streaming protocols and sideband signals, see the [Streaming Data Interfaces](/Tutorials/Features/hls_flow_interfaces/streaming_data_interfaces) sample.

### Kernel

A `LoopBackKernel` processes a fixed number of elements in a loop. On each iteration it:

1. Performs a **blocking read** from `InputPipe` to get the next data element, which is guaranteed to arrive.
2. Performs a **non-blocking read** from `OffsetPipe`: if an offset value is available it is applied, otherwise a default of 0 is used.
3. Computes a result and performs a **blocking write** to `OutputPipe`.

The `OffsetPipe` illustrates a common pattern where auxiliary data arrives at unpredictable intervals. By using a non-blocking read, the kernel can check for new offsets without stalling when none are available. For an example that combines non-blocking streaming pipes with CSR pipe control signals, see the [CSR Pipes](../csr_pipes/) example.

### Host-Side Testbench

The host testbench simulates an external system that drives the kernel:

- Pre-fills all `kCount` elements into `InputPipe` (blocking writes).
- Pre-fills only `kCount/4` offset values into `OffsetPipe`, simulating data that arrives only during the first four reads.
- Launches the kernel.
- Reads all `kCount` results from `OutputPipe` (blocking reads).

## Example Output

```
Running on device: ...
Processing 16 elements
PASSED
```

## Related Samples

- For configuring Avalon and AXI streaming sideband signals on host pipe interfaces, see the [Streaming Data Interfaces](/Tutorials/Features/hls_flow_interfaces/streaming_data_interfaces) sample.
- For inter-kernel pipes that transfer data between two kernels on the device, see the [Inter-Kernel Pipes](../inter_kernel) example.
- For CSR-mapped host pipes used as control signals, see the [CSR Pipes](../csr_pipes) example.
