# `device_global` Sample

This sample is an FPGA tutorial that explains how to use `device_global` class as a way of keeping a state between multiple invocations of a kernel.

| Area                | Description
|:---                 |:---
| What you will learn | The basic usage of the `device_global` class <br> How to initialize a `device_global` to non-zero values<br>
| Time to complete    | 15 minutes
| Category            | Concepts and Functionality

## Purpose

This tutorial demonstrates a simple example of initializing a `device_global` class to a non-zero value, and how to use this approach to keep state between multiple re-launches of a kernel.

## Prerequisites

| Optimized for      | Description
|:---                |:---
| OS                 | Ubuntu* 20.04, Ubuntu* 22.04, Ubuntu* 24.04 <br> RHEL* 9 <br> SUSE* 15 <br> **NOTE: Windows is not supported**
| Hardware           | Agilex® 3, Agilex® 5, Agilex® 7, Stratix® 10 and Arria® 10 FPGAs
| Software           | HLS IP Gen Compiler

> **Note**: Even though the HLS IP Gen compiler is enough to compile for emulation, generating reports and generating RTL, there are extra software requirements for the simulation flow and FPGA compiles.
>
> For using the simulator flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) and one of the following simulators must be installed and accessible through your PATH:
> - Questa*-Altera® FPGA Edition
> - Questa*-Altera® FPGA Starter Edition
> - ModelSim® SE
>
> When using the hardware compile flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) must be installed and accessible through your PATH.

> **Warning** Make sure you add the device files associated with the FPGA that you are targeting to your Quartus® Prime installation.

This sample is part of the FPGA code samples.
It is categorized as a Tier 2 sample that demonstrates a compiler feature.

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

This sample illustrates some key concepts.

- The basic usage of the `device_global` class
- How to initialize a `device_global` to non-zero value

### `device_global` Description

The `device_global` class is an extension that introduces device-scoped memory allocations into SYCL that can be accessed within a kernel using syntax similar to C++ global variables; the class has unique instances per `sycl::device`. Similar to C++ global variables, a `device_global` variable has a namespace scope and is visible to all kernels within that scope.

A `device_global` class is instantiated from a class template. The template is parameterized by the type of the underlying allocation, and a list of properties. The type of the allocation also encodes the size of the allocation, for example in this code sample the `device_global` is templated on `int[kVectorSize]`, which will instantiate a memory of size `sizeof(int) * kVectorSize`. The list of properties lets you control the functional behavior of the `device_global` instance to enable compiler and runtime optimizations. In this code sample two properties are used, `device_image_scope` and `host_access_write`. 

* The `device_image_scope` property limits the scope of a single instance of a `device_global` from a device to a `device_image`. The `device_image_scope` property is required. 

* The `host_access` property tells the compiler how the host code accesses the `device_global`. The property comes in four variants `host_access_none`, `host_access_read`, `host_access_write`, and `host_access_read_write`(the default). The `host_access` property makes no assertion on how the **device** can access the `device_global`: the device can always read and write to the `device_global` object.

> **Note**: Further details on these and other properties can be found in the [device_global Extension](https://docs.altera.com/r/docs/m615048/current/hls-ip-gen-handbook/15.4-persistent-memory-storage-device_global) section of the HLS IP Gen Handbook.

A `device_global` instance can be used to store state across multiple relaunches of a kernel without needing a SYCL buffer or a Unified Shared Memory (USM) pointer. This can be useful for creating a finite state machine.

### Initialize a `device_global` Instance

A `device_global` instance is always zero-initialized, so the compiler cannot pre-initialize `device_global` memories to non-zero values for you at compile-time. However, if you are using a BSP that supports `device_global` memories with host access from a dedicated interfaces, or if you are creating an FPGA IP, you can use the `sycl::queue::copy()` function to copy values from the host to the `device_global` before you start your kernel. These copy operations get placed in the SYCL queue, but will not implicitly block any other operations on the queue.

```cpp
namespace exp = sycl::ext::oneapi::experimental;
using FPGAProperties = decltype(exp::properties(
    exp::device_image_scope, exp::host_access_read_write));
// Declared at namespace scope so visible to all kernels in that scope
exp::device_global<int, FPGAProperties> val;
int main () {
  sycl::queue q;
  int x = 42;
  q.copy(&x, val).wait(); // Write to device_global from x
  q.single_task([=] {
   // Read or Write to device_global
  }).wait();
  q.copy(val, &x).wait(); // Read from device_global into x
}
```
>**Note**: `sycl::queue::copy()` currently only works in the SYCL HLS flow since there is no BSP that supports a dedicated interface for accessing a `device global`.

## Build the `device_global` Tutorial

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
   cmake ..
   ```
   > **Note**: You can change the default target by using the following command. **Targeting a BSP is not supported.**
   >  ```
   >  cmake .. -DFPGA_DEVICE=<FPGA device family or FPGA part number>
   >  ```
   > This tutorial only uses the SYCL HLS flow since there is no BSP that supports a dedicated interface for accessing a `device global`.

3. Compile the design. (The provided targets match the recommended development flow.)

   1. Compile and run for emulation (fast compile time, targets emulates an FPGA device).
      ```
      make fpga_emu
      ```
   2. Generate the HTML optimization reports. The report resides at `device_global.report.prj\reports\report.html`.
      ```
      make report
      ```
   3. Compile for simulation (fast compile time, targets simulated FPGA device).
      ```
      make fpga_sim
      ```
   4. Compile and run on FPGA hardware (longer compile time, targets an FPGA device).
      ```
      make fpga
      ```

## Run the `device_global` Sample

### On Linux

1. Run the sample on the FPGA emulator (the kernel executes on the CPU).
   ```
   ./device_global.fpga_emu
   ```
2. Run the sample on the FPGA simulator device.
   ```bash
   CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 ./device_global.fpga_sim
   ```
   > **Note**: Running this sample on an actual FPGA device requires a BSP that supports device_globals with host access from a dedicated interface. As there are currently no commercial BSPs with such support, only the SYCL HLS flow is enabled for this code sample.

## Example Output
```
PASSED: The results are correct
```

## License

Code samples are licensed under the MIT license. See
[License.txt](/License.txt) for details.

Third party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
