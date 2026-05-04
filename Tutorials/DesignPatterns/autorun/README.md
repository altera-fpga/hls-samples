# `Autorun Kernels` Sample

This `Autorun Kernels` sample is a tutorial that demonstrates how to create the equivalent of OpenCL `autorun` kernels in oneAPI. An autorun kernel is one that is submitted before the `main()` function begins, and typically (though not necessarily) never finishes.

| Area                | Description
|:---                 |:---
| What you will learn | How and when to use autorun kernels
| Time to complete    | 15 minutes
| Category            | Code Optimization

## Purpose

The purpose of this tutorial is to demonstrate how to create autorun kernels in SYCL*-compliant code. Autorun kernels are submitted to the SYCL queue automatically before `main()` begins and are meant to have no direct interaction with the host. This means that they should not directly access global memory (for example, through USM pointers or SYCL accessors) and the host should not wait for them to finish.

## Prerequisites

This sample is part of the FPGA code samples.
It is categorized as a Tier 3 sample that demonstrates a design pattern.

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
You can also find more information about [troubleshooting build errors](/README.md#troubleshooting), [links to selected documentation](/README.md#documentation), etc.

| Optimized for      | Description
|:---                |:---
| OS                 | Ubuntu* 20.04, Ubuntu* 22.04, Ubuntu* 24.04 <br> RHEL* 8, RHEL* 9 <br> SUSE* 15 <br> **NOTE: Windows is not supported**
| Hardware           | Agilex® 3, Agilex® 5, Agilex® 7, Stratix® 10 and Arria® 10 FPGAs
| Software           | HLS IP Gen Compiler

> **Note**: Even though the HLS IP Gen compiler is enough to compile for emulation, generating reports and generating RTL, there are extra software requirements for the simulation flow and FPGA compiles.
>
> For using the simulator flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) and one of the following simulators must be installed and accessible through your PATH:
> - Questa*-Intel® FPGA Edition
> - Questa*-Intel® FPGA Starter Edition
> - ModelSim® SE
>
> When using the hardware compile flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) must be installed and accessible through your PATH.
>
> :warning: Make sure you add the device files associated with the FPGA that you are targeting to your Quartus® Prime installation.

## Key Implementation Details

This sample demonstrates the following concepts:

- How to use the autorun kernel header file (`autorun.hpp`).
- When it is appropriate to use autorun kernels.

Typically, these kernels are meant to run forever, and data is streamed to and from them using SYCL pipes. This technique is illustrated in the figures below where the middle light-blue kernels (`ARKernel` and `ARForeverKernel`) are autorun kernels, and the dark-blue kernels on the left and right are regular SYCL kernels. The images below correspond to the `Autorun` and `AutorunForever` kernels used in this tutorial. See the comments and code in `autorun.cpp` for more details on the differences.

![autorun](assets/autorun.png)

![autorun forever](assets/autorun_forever.png)

## Build the `Autorun Kernels` Sample

> **Note**: When working with the command-line interface (CLI), you should configure the HLS IP Gen Compiler using environment variables.
> Set up your CLI environment by sourcing the `fpgavars` script located in the root of your HLS IP Gen Compiler installation every time you open a new terminal window.
> This practice ensures that your compiler, libraries, and tools are ready for development.
>
> Linux*:
> - `source <install-dir>/fpgavars.sh`
> - For non-POSIX shells, like csh, use the following command: `bash -c 'source <install-dir>/fpgavars.sh ; exec csh'`

### On Linux*

1. Change to the sample directory.
2. Build the program for the Agilex® 7 device family, which is the default.

   ```
   mkdir build
   cd build
   cmake ..
   ```

   > **Note**: You can change the default target by using the following command. **Targeting a BSP is not supported.**
   >  ```
   >  cmake .. -DFPGA_DEVICE=<FPGA device family or FPGA part number>
   >  ```

3. Compile the design. (The provided targets match the recommended development flow.)

   1. Compile for emulation (fast compile time, targets emulated FPGA device).
      ```
      make fpga_emu
      ```
   2. Generate HTML performance report.
      ```
      make report
      ```
      The report resides at `autorun.report.prj/reports/report.html`.

   3. Compile for simulation (fast compile time, targets simulated FPGA device).
      ```
      make fpga_sim
      ```
   4. Compile for FPGA hardware (longer compile time, targets FPGA device).
      ```
      make fpga
      ```

## Run the `Autorun Kernels` Sample

### On Linux

1. Run the FPGA emulator (the kernel executes on the CPU).
   ```
   ./autorun.fpga_emu
   ```
2. Run on the FPGA simulator.
   ```
   CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 ./autorun.fpga_sim
   ```
## Example Output

```
Running the Autorun kernel test
Running the AutorunForever kernel test
PASSED
```

## License

Code samples are licensed under the MIT license. See [License.txt](/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
