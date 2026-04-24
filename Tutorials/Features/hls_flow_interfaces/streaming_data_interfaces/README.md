# `Streaming Data Interfaces` Sample

This FPGA sample is a tutorial that demonstrates how to implement streaming data interfaces on an IP component. It is recommended that you review the [Host Pipes](/Tutorials/Features/experimental/hostpipes) tutorial and the [Component Interfaces Comparison](/Tutorials/Features/hls_flow_interfaces/component_interfaces_comparison) tutorial, specifically the [pipes](/Tutorials/Features/hls_flow_interfaces/component_interfaces_comparison/pipes/) code sample, before continuing with this one.

| Area                  | Description
|:--                    |:--
| What you will learn   | How to use pipes to implement streaming data interfaces on an IP component
| Time to complete      | 30 minutes
| Category              | Concepts and Functionality


## Purpose

Streaming pipes are first-in first-out (FIFO) buffer constructs that provide data links between elements of a design. They are accessed through read and write APIs without the notion of a memory address or pointers to elements within the FIFO.

The concept of a streaming pipe is an intuitive mechanism for specifying streaming data interfaces on an IP component. This tutorial demonstrates how to use the pipe API to configure a streaming interface.

## Prerequisites

| Optimized for        | Description
|:---                  |:---
| OS                   | Ubuntu* 20.04, Ubuntu* 22.04, Ubuntu* 24.04 <br> RHEL* 8, RHEL* 9 <br> SUSE* 15 <br> **NOTE: Windows is not supported**
| Hardware             | Agilex® 5, Agilex® 7 and Arria® 10 FPGAs
| Software             | HLS IP Gen Compiler

> **Note:** Even though the HLS IP Gen compiler is enough to compile for emulation, generating reports and generating RTL, there are extra software requirements for the simulation flow and FPGA compiles.
>
> For using the simulator flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) and one of the following simulators must be installed and accessible through your PATH:
> - Questa*-Intel® FPGA Edition
> - Questa*-Intel® FPGA Starter Edition
> - ModelSim® SE
>
> When using the hardware compile flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) must be installed and accessible through your PATH.

> **Warning:** Make sure you add the device files associated with the FPGA that you are targeting to your Quartus® Prime installation.

This sample is part of the FPGA code samples. It is categorized as a Tier 2 sample that demonstrates a compiler feature.

```mermaid
flowchart LR
   tier1("Tier 1: Get Started")
   tier2("Tier 2: Explore the Fundamentals")
   tier3("Tier 3: Explore the Advanced Techniques")
   tier4("Tier 4: Explore the Reference Designs")

   tier1 --> tier2 --> tier3 --> tier4

   style tier1 fill:#0071c1,stroke:#0071c1,stroke-width:1px,color:#fff
   style tier2 fill:#f96,stroke:#333,stroke-width:1px,color:#fff
   style tier3 fill:#0071c1,stroke:#0071c1,stroke-width:1px,color:#fff
   style tier4 fill:#0071c1,stroke:#0071c1,stroke-width:1px,color:#fff
```

Find more information about how to navigate this part of the code samples in the [FPGA top-level README.md](/README.md).
You can also find more information about [troubleshooting build errors](/README.md#troubleshooting), [links to selected documentation](/README.md#documentation), and more.


## Key Implementation Details

### Configuring a Streaming Pipe

Each pipe is a class declaration of the templated `pipe` class. A pipe declaration takes two mandatory and two optional parameters, as summarized in [Table 1](#table-1-template-parameters-of-the-pipe-class).

#### Table 1. Template Parameters of the `pipe` Class

| Template Parameter                      | Description
| ---                                     | ---
| `name`                                  | A user-defined type that uniquely identifies this pipe. The name of this type is also used to identify the interface in the generated RTL.
| `dataT`                                 | The type of data that passes through the pipe*. This is the data type that is read during a successful pipe read() operation, or written during a successful pipe write() operation. The type must have a standard layout and be trivially copyable.
| `min_capacity`                          | User-defined minimum number of words (in units of `dataT`) that the pipe must be able to store without any being read out. This parameter is optional, and defaults to 0.
| `properties`                            | An unordered list of SYCL properties that define additional semantic properties for a pipe. This parameter is optional.**

> **Note**: Omitting a single property from the properties class instructs the compiler to assume the default value for that property, so you can just define the properties you would like to change from the default. Omitting the properties template parameter entirely instructs the compiler to assume the default values for all properties.

Below is a summary of all relevant SYCL properties which can be applied to a `pipe` using the `properties` template parameter. Please note that this table is not complete; see the [HLS IP Gen Handbook](https://www.intel.com/content/www/us/en/docs/oneapi-fpga-add-on/optimization-guide/current/host-pipe-declaration.html) for more information on how to use pipes in other applications.

#### Table 2. Properties used to Configure a Pipe to Implement a Streaming Data Interface

| Property                                | Default Value               | Valid Values
| ---                                     | ---                         | ---
| `ready_latency<uint32_t>`               | 0                           | non-negative integer
| `bits_per_symbol<uint32_t>`             | 8                           | non-negative integer that divides the size of the data type
| `uses_valid<bool>`                      | `true`                      | boolean
| `uses_ready<bool>`                      | `true`                      | boolean
| `first_symbol_in_high_order_bits<bool>` | `false`                     | boolean
| `protocol`                              | `protocol_avalon_streaming` | `protocol_avalon_streaming` / `protocol_avalon_mm` / `protocol_axi_streaming`

To configure a pipe to implement a streaming data interface, specify either `protocol_avalon_streaming` or `protocol_axi_streaming` for the `protocol` property. The `protocol_avalon_mm` pipe protocol does not generate a streaming data interface; it configures host pipes as a CSR (memory-mapped agent) data interface instead. For details and a full example on configuring a CSR data interface, see the [CSR Data](/Tutorials/Features/hls_flow_interfaces/component_interfaces_comparison/csr-pipes/) code sample.

> **Note**: The pipe's `properties` template are largely based on Avalon® streaming interface semantics (e.g., ready latency, valid/ready usage, and bits per symbol). Currently AXI™ streaming does not add a separate set of protocol-specific properties in the pipe's `properties` template. For more information on Avalon® streaming interface properties, see [this page](https://docs.altera.com/r/docs/683091/22.3/avalon-interface-specifications/avalon-st-interface-properties).

#### Example 1.

The following example explicitly declares a pipe that implements an Avalon® streaming data interface with `ready` and `valid` signals. All the properties specified in `DefaultPropertiesT` are default, including the `protocol` property.

```c++
// Forward declare the pipe name in the global scope. This is an FPGA best
// practice that reduces name mangling in the optimization reports.
class FirstPipeT;

// Pipe properties (listed here are the defaults; this achieves the same
// behavior as not specifying any of these properties)
using DefaultPropertiesT = decltype(sycl::ext::oneapi::experimental::properties(
    sycl::ext::altera::experimental::ready_latency<0>,
    sycl::ext::altera::experimental::bits_per_symbol<8>,
    sycl::ext::altera::experimental::uses_valid<true>,
    sycl::ext::altera::experimental::uses_ready<true>,
    sycl::ext::altera::experimental::first_symbol_in_high_order_bits<false>,
    sycl::ext::altera::experimental::protocol_avalon_streaming));

using FirstPipe = sycl::ext::altera::experimental::pipe<
    FirstPipeT,          // An identifier for the pipe
    int,                 // The type of data in the pipe
    8,                   // Minimum capacity of the pipe (buffer depth)
    DefaultPropertiesT   // Pipe properties, customizable
    >;
```

### Pipe API

Pipes expose read and write interfaces that allow data to be read or written in FIFO order to the pipe.

See the [Host Pipes](/Tutorials/Features/experimental/hostpipes) code sample for more details on the read and write APIs.

### Streaming Sideband Signals

Besides the main data payload and the usual valid/ready handshake, streaming interfaces can expose extra signals to denote variable-length packet boundaries or specify custom metadata. Those signals are called *sideband* signals, and are protocol-dependent. For pipes that implement a streaming data interface, you enable them by using a per-protocol, dedicated *beat* type as the pipe's `dataT`:

  - `StreamingBeat` for Avalon® streaming (`protocol_avalon_streaming`)
  - `StreamingBeatAxi` for AXI™ streaming (`protocol_axi_streaming`)

Only these beat types cause the compiler to infer the corresponding sideband wires in the generated interface.

### Sample Structure

There are 2 different example designs in this sample, both of which implement a pixel clipping kernel intaking pixel beats with sideband signals. The source code demonstrates how to use streaming sideband signals for the Avalon® streaming data interface and the AXI™ streaming data interface.

  1. [Avalon® streaming data interface](avalon_streaming/) This design uses `StreamingBeat` as the pipe element type and `protocol_avalon_streaming` in the pipe properties. It exercises Avalon® streaming sideband signals (`startofpacket` and `endofpacket`; the `empty` signal is disabled in this example) while streaming pixels through host pipes.
  2. [AXI™ streaming data interface](axi_streaming/) This design uses `StreamingBeatAxi` as the pipe element type and `protocol_axi_streaming` in the pipe properties. It exercises AXI™ streaming sideband signals (`tlast` by default; `tuser` is not enabled in this example) for the same pixel-clipping kernel pattern.

## Build the `Streaming Data Interfaces` Tutorial

>**Note**: When working with the command-line interface (CLI), you should configure the HLS IP Gen Compiler using environment variables. Set up your CLI environment by sourcing the `fpgavars` script in the root of your HLS IP Gen Compiler installation every time you open a new terminal window. This practice ensures that your compiler, libraries, and tools are ready for development.
>
> Linux*:
> - `source <install-dir>/fpgavars.sh`
> - For non-POSIX shells, like csh, use the following command: `bash -c 'source <install-dir>/fpgavars.sh ; exec csh'`

### On Linux*

1. Change to the sample directory.
2. Build the program for Agilex® 7 device family, which is the default.
   ```
   mkdir build
   cd build
   cmake .. -DTYPE=<AVST/AXIST>
   ```
   > **Note**: You can change the default target by using the following command. **Targeting a BSP is not supported.**
   >  ```
   >  cmake .. -DFPGA_DEVICE=<FPGA device family or FPGA part number> -DTYPE=<AVST/AXIST>
   >  ```
   >
   > This tutorial is only intended for use in the SYCL HLS flow and does not support targeting an explicit FPGA board variant and BSP.

3. Compile the design. (The provided targets match the recommended development flow.)

   1. Compile and run for emulation (fast compile time, targets emulated FPGA device).
      ```
      make fpga_emu
      ```
      >**Note**: Since this design uses host pipes, make sure that the emulator pipe depth behaviour is as intended. Set the environment variable `CL_CONFIG_CHANNEL_DEPTH_EMULATION_MODE` to `ignore-depth` for this design so that multiple writes can happen to the pipe without first having the contents read.

   2. Generate the optimization report.
      ```
      make report
      ```
   3. Compile and run for simulation (fast compile time, targets simulated FPGA device).
      ```
      make fpga_sim
      ```
   4. Compile for FPGA hardware (longer compile time, targets an FPGA device).
      ```
      make fpga
      ```	

## Run the `Streaming Data Interfaces` Tutorial

### On Linux

1. Run the sample on the FPGA emulator (the kernel executes on the CPU).
   ```
   ./streaming_data_interfaces.fpga_emu
   ```
2. Run the sample on the FPGA simulator.
   ```
   CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 ./streaming_data_interfaces.fpga_sim
   ```


## License

Code samples are licensed under the MIT license. See [License.txt](/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
