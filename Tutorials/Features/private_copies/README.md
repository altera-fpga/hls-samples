# `private_copies` Sample

This sample is an FPGA tutorial that explains how to use the `private_copies` attribute to trade off the on-chip memory use and the throughput of a SYCL*-compliant FPGA program.


| Area                 | Description
|:--                   |:--
| What you will learn  | The basic usage of the `private_copies` attribute. <br> How the `private_copies` attribute affects the throughput and resource use of your FPGA program. <br> How to apply the `private_copies` attribute to variables or arrays in your program. <br> How to identify the correct `private_copies` factor for your program.
| Time to complete     | 15 minutes
| Category             | Concepts and Functionality

## Purpose

This tutorial demonstrates a simple example of applying the `private_copies` attribute to an array within a loop in a task kernel to trade off the on-chip memory use and throughput of the loop.

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

- The basic usage of the `private_copies` attribute
- How the `private_copies` attribute affects the throughput and resource use of your SYCL-compliant FPGA program
- How to apply the `private_copies` attribute to variables or arrays in your program
- How to identify the correct `private_copies` factor for your program

### Description of the `private_copies` Attribute

The `private_copies` attribute is a memory attribute that enables you to control the number of private copies of any variable or array declared inside a pipelined loop. These private copies allow multiple iterations of the loop to run concurrently by providing them their own private copies of arrays to operate on. The number of concurrent loop iterations is limited by the number of private copies specified by the `private_copies` attribute.

Kernels in this tutorial design apply `[[intel::private_copies(N)]]` to an array declared within an outer loop and used by subsequent inner loops. These inner loops perform a global memory access before storing the results. The following is an example of such a loop:

```cpp
for (size_t i = 0; i < kMaxIter; i++) {
  [[intel::private_copies(2)]] int a[kSize];
  for (size_t j = 0; j < kSize; j++) {
    a[j] = accessor_array[(i * 4 + j) % kSize] * shift;
  }
  for (size_t j = 0; j < kSize / 2; j++)
    r += a[j];
}
```

In this example, you only need to have two private copies of array `a` in order to have 2 concurrent outer loop iterations. The `private_copies` attribute in this example forces the compiler to create two private copies of the array `a`. In general, passing the parameter `N` to the `private_copies` attribute limits the number of private copies created for array `a` to `N`, which in turn limits the concurrency of the outer loop to `N`.

### Identifying the Correct `private_copies` Factor

Generally, increasing the number of private copies of an array within a loop situated in a task kernel will increase the throughput of that loop at the cost of increased memory use. However, in most cases, there is a limit beyond which increasing the number of private copies does not have any further effect on the throughput of the loop. That limit is the maximum exploitable concurrency of the outer loop.

The correct `private_copies` factor for a given array depends on your goals for the design, the criticality of the loop in question, and its impact on your design's overall throughput. In the example above we can analytically determine what the value of `private_copies` should be by looking at the structure of the nested loops. The two nested inner loops both require memory accesses to `a`, meaning that before a second outer loop iteration can start we would have to wait for the second inner loop to finish running. Using a `private_copies` value of 2 allows for the second outer loop iteration to start as soon as the first inner loop finishes, allowing both inner loops to run in parallel.

A typical design flow may be to:

1. Analyze your code to come up with an estimate of the number of `private_copies` that should give you the desired throughput and area for your design. Alternatively, you can rely on the compilers default heuristic, setting `private_copies` to 0, to assist with this. If choosing this option, note that the compiler heuristic might sometimes be wrong. You might need to rely on your own estimation.
2. Observe what impact the values have on the overall throughput and memory use of your design.
3. Choose the appropriate value that allows you to achieve your desired throughput and area goals.

## Build the `private_copies` Tutorial

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

### Read the Reports

Locate `report.html` in the `private_copies.report.prj/reports/` directory.

On the main report page, scroll down to the section titled "Estimated Resource Usage". Each kernel name ends in the `private_copies` attribute argument used for that kernel, e.g., `kernelCompute1` uses a `private_copies` attribute value of 1. You can verify that the number of RAMs used for each kernel increases with the `private_copies` value used, except for `private_copies` 0. Using `private_copies` 0 instructs the compiler to choose a default value, which is often near the value that would give you maximum throughput.

## Run the `private_copies` Sample

### On Linux

1. Run the sample on the FPGA emulator (the kernel executes on the CPU).
   ```
   ./private_copies.fpga_emu
   ```
2. Run the sample on the FPGA simulator device.
   ```
   CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 ./private_copies.fpga_sim
   ```
## Example Output

```
Running on device: ofs_n6001 : Intel OFS Platform (ofs_ee00000)
Kernel time when private_copies is set to 0: 682.683 ms
Kernel throughput when private_copies is set to 0: 1.200 GFlops
Kernel time when private_copies is set to 1: 1080.958 ms
Kernel throughput when private_copies is set to 1: 0.758 GFlops
Kernel time when private_copies is set to 2: 682.684 ms
Kernel throughput when private_copies is set to 2: 1.200 GFlops
Kernel time when private_copies is set to 3: 682.684 ms
Kernel throughput when private_copies is set to 3: 1.200 GFlops
Kernel time when private_copies is set to 4: 682.684 ms
Kernel throughput when private_copies is set to 4: 1.200 GFlops
PASSED: The results are correct
```

The stdout output shows the throughput (GFlops) for each kernel.

When run on the Intel® FPGA SmartNIC N6001-PL, we see that the throughput of the kernel approximately doubles when going from 1 to 2 private copies for array `a`. Increasing to 3 private copies has a very small effect, and further increasing the number of private copies does not increase the throughput achieved. This means that increasing private copies beyond 2 will incur an area penalty for little or no throughput gain. As such, for this tutorial design, optimal throughput/area tradeoff is achieved when using 2 private copies. This is as expected based on an analysis of the code, as there are two inner loops which are able to run in parallel if they each have their own private copy of the array. The small jump in throughput observed from using 3 private copies instead of 2 can be attributed to the third private copy allowing subsequent outer loop iterations to launch slightly sooner than they would with 2 private copies due to internal delays in tracking the use of each private copy.

Setting the `private_copies` attribute to 0 (or equivalently omitting the attribute entirely) produced good throughput, and the reports show us that the compiler selected 3 private copies. This does produce the optimal throughput, but in this case it probably makes sense to save some area in exchange for a very small throughput loss by specifying 2 private copies instead.

When run on the FPGA emulator or simulator, the `private_copies` attribute has no effect on kernel time. You may actually notice that the emulator achieved higher throughput than the FPGA in this example. This is because this trivial example uses only a tiny fraction of the spatial compute resources available on the FPGA.

## License

Code samples are licensed under the MIT license. See [License.txt](/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
