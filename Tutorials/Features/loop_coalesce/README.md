
# `loop_coalesce` Sample

This sample is an FPGA tutorial that demonstrates applying the `loop_coalesce` attribute to a nested loop in a task kernel to reduce the area overhead.

| Area                 | Description
|:--                   |:--
| What you will learn  |  What the `loop_coalesce` attribute does. <br> How `loop_coalesce` attribute affects resource usage and loop throughput. <br> How to apply the `loop_coalesce` attribute to loops in your program. <br> Which loops make good candidates for coalescing.
| Time to complete     | 15 minutes
| Category             | Concepts and Functionality

## Purpose

The `loop_coalesce` attribute enables you to direct the compiler to combine nested loops into a single loop. The attribute `[[intel::loop_coalesce(N)]]` takes an integer argument `N` that specifies how many nested loop levels that you want the compiler to attempt to coalesce.

>**Note**: If you specify `[[intel::loop_coalesce(1)]]` on nested loops, the compiler does not attempt to coalesce any of the nested loops.

## Prerequisites

| Optimized for        | Description
|:---                  |:---
| OS                   | Ubuntu* 20.04, Ubuntu* 22.04, Ubuntu* 24.04 <br> RHEL* 9 <br> SUSE* 15 <br> **NOTE: Windows is not supported**
| Hardware             | Agilex® 3, Agilex® 5, Agilex® 7, Stratix® 10 and Arria® 10 FPGAs
| Software             | HLS IP Gen Compiler

> **Note**: Even though the HLS IP Gen compiler is enough to compile for emulation, generating reports and generating RTL, there are extra software requirements for the simulation flow and FPGA compiles.
>
> For using the simulator flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) and one of the following simulators must be installed and accessible through your PATH:
> - Questa*-Altera® FPGA Edition
> - Questa*-Altera® FPGA Starter Edition
> - ModelSim® SE
>
> When using the hardware compile flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) must be installed and accessible through your PATH.

> **Warning**: Make sure you add the device files associated with the FPGA that you are targeting to your Quartus® Prime installation.

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

The sample illustrates the important concepts.

- Describing the `loop_coalesce` attribute.
- Showing the effects of the `loop_coalesce` attribute on resource usage and loop throughput.
- Applying the `loop_coalesce` attribute to loops to your program.
- Determining which loops make good candidates for coalescing.

### Coalescing Two Loops

The following examples show how to coalesce two loops.

```
[[intel::loop_coalesce(2)]]
for (int i = 0; i < N; i++)
  for (int j = 0; j < M; j++)
    sum[i][j] += i+j;
```
The compiler coalesces the two loops together so that they execute as if they were a single loop written as follows:

```
int i = 0;
int j = 0;
while(i < N){
  sum[i][j] += i+j;
  j++;
  if (j == M){
    j = 0;
    i++;
  }
}
```

### Identifying Which Loops to Coalesce

Generally, coalescing loops can help reduce area usage by reducing the overhead needed for loop control. However, in some circumstances, coalescing loops can reduce kernel throughput. Scenarios where the `loop_coalesce` attribute can be applied to save area without a loss of throughput, are those where:

   1. The loops being coalesced have the same initiation interval (II).
   2. The exit condition computation for the resulting coalesced look is not complicated.

If the innermost coalesced loop has a very small trip count, `loop_coalesce` might actually improve throughput.

## Build the `Loop Coalesce` Tutorial

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
Locate `report.html` in the `loop_coalesce.report.prj/reports/` directory. 

On the main report page, scroll down to the section titled `Compile Estimated Kernel Resource Utilization Summary`. Each kernel name ends in the `loop_coalesce` attribute argument used for that kernel; for example, KernelCompute<2> uses a `loop_coalesce` argument of `2`. You can verify that the number of ALMs used decreases when the loops are coalesced. Since KernelCompute<2> has fewer loops than KernelCompute<1>, it requires less hardware for loop overhead. 

## Run the `Loop Coalesce` Sample

### On Linux

1. Run the sample on the FPGA emulator (the kernel executes on the CPU).
   ```
   ./loop_coalesce.fpga_emu
   ```
2. Run the sample on the FPGA simulator device.
   ```
   CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 ./loop_coalesce.fpga_sim
   ```
## License

Code samples are licensed under the MIT license. See [License.txt](/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
