# `Stall Enable` Sample

This sample is an FPGA tutorial that demonstrates how to use the `use_stall_enable_clusters` attribute to reduce the area and computation latency of your FPGA kernels. This attribute may reduce the FPGA FMax for your kernels.

| Area                 | Description
|:---                  |:---
| What you will learn  |  What the `use_stall_enable_clusters` attribute does. <br> How `use_stall_enable_clusters` attribute affects resource usage and latency. <br> How to apply the `use_stall_enable_clusters` attribute to kernels in your program.
| Time to complete     | 15 minutes (emulator and report) <br> several hours (fpga bitstream generation)
| Category             | Concepts and Functionality

## Purpose

The `use_stall_enable_clusters` attribute enables you to direct the compiler to reduce the area and latency of your kernel.  Reducing the latency will not have a large effect on loops that are pipelined, unless the number of iterations of the loop is very small.
Computations in an FPGA kernel are normally grouped into *Stall Free Clusters*. This allows simplification of the signals within the cluster, but there is a FIFO queue at the end of the cluster that is used to save intermediate results if the computation needs to stall. *Stall Enable Clusters* save area and cycles by removing the FIFO queue and passing the stall signals to each part of the computation.  These extra signals may cause the FMax to be reduced.

> **Note**: If you specify `[[intel::use_stall_enable_clusters]]` on one or more kernels, this may reduce the FMax of the generated FPGA bitstream, which may reduce performance on all kernels.

> **Note**: The `use_stall_enable_clusters` attribute is not applicable for designs that target FPGA architectures that support hyper optimized handshaking, unless the `-Xshyper-optimized-handshaking=off` argument is passed to `icpx`.

## Prerequisites

| Optimized for        | Description
|:---                  |:---
| OS                   | Ubuntu* 20.04, Ubuntu* 22.04, Ubuntu* 24.04 <br> RHEL* 8, RHEL* 9 <br> SUSE* 15 <br> **NOTE: Windows is not supported**
| Hardware             | Agilex® 5, Agilex® 7 and Arria® 10 FPGAs
| Software             | HLS IP Gen Compiler

> **Note**: Even though the HLS IP Gen compiler is enough to compile for emulation, generating reports and generating RTL, there are extra software requirements for the simulation flow and FPGA compiles.
>
> For using the simulator flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) and one of the following simulators must be installed and accessible through your PATH:
> - Questa*-Intel® FPGA Edition
> - Questa*-Intel® FPGA Starter Edition
> - ModelSim® SE
>
> When using the hardware compile flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) must be installed and accessible through your PATH.

> **Warning**: Make sure you add the device files associated with the FPGA that you are targeting to your Quartus® Prime installation.

This sample is part of the FPGA code samples.
It is categorized as a Tier 3 sample that demonstrates a compiler feature.

```mermaid
flowchart LR
   tier1("Tier 1: Get Started")
   tier2("Tier 2: Explore the Fundamentals")
   tier3("Tier 3: Explore the Advanced Techniques")
   tier4("Tier 4: Explore the Reference Designs")

   tier1 --> tier2 --> tier3 --> tier4

   style tier1 fill:#0071c1,stroke:#0071c1,stroke-width:1px,color:#fff
   style tier2 fill:#0071c1,stroke:#0071c1,stroke-width:1px,color:#fff
   style tier3 fill:#f96,stroke:#333,stroke-width:1px,color:#fff
   style tier4 fill:#0071c1,stroke:#0071c1,stroke-width:1px,color:#fff
```

Find more information about how to navigate this part of the code samples in the [FPGA top-level README.md](/README.md).
You can also find more information about [troubleshooting build errors](/README.md#troubleshooting), [links to selected documentation](/README.md#documentation), and more.

## Key Implementation Details

The sample illustrates the following important concepts.

- Description of the `use_stall_enable_clusters` attribute
- How `use_stall_enable_clusters` attribute affects resource usage and loop throughput
- How to apply the `use_stall_enable_clusters` attribute to kernels in your program

### Example of Using the `use_stall_enable_clusters` Attribute

```c++
h.single_task<class KernelComputeStallFree>( [=]() [[intel::use_stall_enable_clusters]] {
  // The computations in this device kernel will use Stall Enable clusters
  Work(accessor_vec_a, accessor_vec_b, accessor_res);
});
```

The FPGA compiler will use *Stall Enable Clusters* for the kernel when possible.  Some computations may not be able to stall and need to be placed in a *Stall Free Cluster*.

## Build the `Stall Enable` Tutorial

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
   cmake .. -DPART=<X>
   ```
   where `X` is:
   - `STALL_ENABLE`
   - `STALL_FREE`
   > **Note**: You can change the default target by using the following command. **Targeting a BSP is not supported.**
   >  ```
   >  cmake .. -DFPGA_DEVICE=<FPGA device family or FPGA part number>
   >  ```

   > **Note**: For targets that support hyper-optimized handshaking, you must disable the handshaking with `-DNO_HYPER_OPTIMIZATION=true`.

3. Compile the design. (The provided targets match the recommended development flow.)

   1. Compile and run for emulation (fast compile time, targets emulates an FPGA device).
      ```
      make fpga_emu
      ```
   2. Generate the HTML optimization reports. (See [Read the Reports](#read-the-reports) below for information on finding and understanding the reports.)
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

## Read the Reports

Locate `report.html` files in both versions of the build (`-DPART=STALL_ENABLE` and `-DPART=STALL_FREE`).

On the main report page, scroll down to the section titled `Compile Estimated Kernel Resource Utilization Summary`. Note that the estimated number of **MLAB**s for KernelComputeStallEnable is smaller than that of KernelComputeStallFree. The reduction in MLABs is due to the elimination of the FIFO queue at the end of the cluster.

## Run the `Stall Enable` Sample

### On Linux

1. Run the sample on the FPGA emulator (the kernel executes on the CPU).
   ```
   ./stall_enable.fpga_emu
   ```
2. Run the sample on the FPGA simulator device.
   ```
   CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 ./stall_enable.fpga_sim
   ```
## Example Output

### Example Output for `stall_enable.fpga_emu` compiled with `-DPART=STALL_FREE`

```
Running on device: ofs_n6001 : Intel OFS Platform (ofs_ee00000)
Stall free Kernel -- kernel time : 12.8 microseconds
Throughput for kernel: 15625001KB/s
PASSED: The results are correct
```

### Example Output for `stall_enable.fpga_emu` compiled with `-DPART=STALL_ENABLE`

```
Running on device: ofs_n6001 : Intel OFS Platform (ofs_ee00000)
Stall enable Kernel -- kernel time : 7.936 microseconds
Throughput for kernel: 25201614KB/s
PASSED: The results are correct
```

There should be no observable difference in results in the example. Only the area and latency differences are visible in the reports.

## License

Code samples are licensed under the MIT license. See [License.txt](/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
