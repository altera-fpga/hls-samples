# Using the Intercept Layer for OpenCL™ Applications to Identify Optimization Opportunities
This FPGA tutorial demonstrates how to use the Intercept Layer for OpenCL™ Applications to perform system-level profiling on a design and reveal areas for improvement.

> **Note**: Tutorial does not work on Windows* as compiling to FPGA hardware is not yet supported in Windows.

The [Intercept Layer for OpenCL™ Applications](https://github.com/intel/opencl-intercept-layer) GitHub provides complete documentation on using this tool.

| Optimized for                     | Description
---                                 |---
| OS                                | Ubuntu* 20.04 <br> RHEL*/CentOS* 8 <br> SUSE* 15
| Hardware                          | Intel® Agilex® 7, Agilex® 5, Arria® 10, Stratix® 10, and Cyclone® V FPGAs
| Software                          | Intel® oneAPI DPC++/C++ Compiler
| What you will learn               | Summary of profiling tools available for performance optimization <br> About the Intercept Layer for OpenCL™ Applications <br> How to set up and use this tool <br> A case study of using this tool to identify when the double buffering system-level optimization is beneficial
| Time to complete                  | 30 minutes

> **Note**: Even though the Intel DPC++/C++ oneAPI compiler is enough to compile for emulation, generating reports and generating RTL, there are extra software requirements for the simulation flow and FPGA compiles.
>
> For using the simulator flow, Intel® Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) and one of the following simulators must be installed and accessible through your PATH:
> - Questa*-Intel® FPGA Edition
> - Questa*-Intel® FPGA Starter Edition
> - ModelSim® SE
>
> When using the hardware compile flow, Intel® Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) must be installed and accessible through your PATH.
>
> :warning: Make sure you add the device files associated with the FPGA that you are targeting to your Intel® Quartus® Prime installation.

## Prerequisites

This sample is part of the FPGA code samples.
It is categorized as a Tier 3 sample that demonstrates the usage of a tool.

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

## Purpose
This FPGA tutorial demonstrates how to use the Intercept Layer for OpenCL™ Applications, an open-source tool, to perform system-level profiling on a design and reveal areas for improvement.
When targeting an FPGA family/part, no FPGA executable is generated. So this sample is really meant to be used when targeting a device with a BSP where an FPGA executable would be produced.

### Profiling Techniques
The following code snippet uses standard SYCL* and C++ language features to extract profiling information from code.

```c++
void profiling_example(const std::vector<float>& vec_in,
                             std::vector<float>& vec_out ) {

  // Start the timer
  auto start = std::chrono::steady_clock::now();

  // Host performs pre-processing of input data
  std::vector<float> vec_pp = PreProcess(vec_in);

  // FPGA device performs additional processing
  ext::intel::fpga_selector selector;
  queue q(selector, fpga_tools::exception_handler,
          property::queue::enable_profiling{});

  buffer buf_in(vec_pp);
  buffer buf_out(vec_out);

  event e = q.submit([&](handler &h) {
    accessor acc_in(buf_in, h, read_only);
    accessor acc_out(buf_out, h, write_only, no_init);

    h.single_task<class Kernel>([=]() [[intel::kernel_args_restrict]] {
      DeviceProcessing(acc_in, acc_out);
    });
  });

  // Query event e for kernel profiling information
  // (blocks until command groups associated with e complete)
  double kernel_time_ns =
    e.get_profiling_info<info::event_profiling::command_end>() -
    e.get_profiling_info<info::event_profiling::command_start>();

  // Stop the timer.
  auto end = std::chrono::steady_clock::now();
  double total_time_s = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();

  // Report profiling info
  std::cout << "Kernel compute time:  " << kernel_time_ns * 1e-6 << " ms\n";
  std::cout << "Total compute time:   " << total_time_s   * 1e3  << " ms\n";
}
```

This tutorial introduces the Intercept Layer for OpenCL™ Applications, a profiling tool that extracts and visualizes system-level profiling information for SYCL-compliant programs.  This tool can extract the same profiling data (and more) as the code snippet above, without requiring any code-level profiling directives.

The Intercept Layer for OpenCL™ provides coarse-grained, system-level profiling information. A complementary tool, the Intel® FPGA Dynamic Profiler for DPC++, provides fine-grained profiling information for the kernels executing on the device. Together, these two tools can be used to optimize both host and device side execution. However, these tools should not be used simultaneously, as the Intercept Layer for OpenCL™ may slow down the runtime execution, rendering the Dynamic Profiler data less accurate.

### The Intercept Layer for OpenCL™ Applications

The Intercept Layer for OpenCL™ Applications is an open-source tool that you can use to profile SYCL-conforming designs at a system-level. Although it is not part of the oneAPI Base Toolkit installation, it is freely available on GitHub.

This tool serves the following purposes:
- Intercept host calls before they reach the device to gather performance data and log host calls.
- Provide data to visualize the calls through time, separating them into *queued*, *submitted*, and *execution* sections to help you better understand the execution.
- Identify gaps (using visualization) in the runtime. Gaps often indicate inefficient execution and throughput loss.

The Intercept Layer for OpenCL™ Applications has several different options for capturing different aspects of the host run. These options are described in its [documentation](https://github.com/intel/opencl-intercept-layer). This tutorial uses the call-logging and device timeline features that print information about the host's calls during execution.

### Data Visualization

You can visualize the data generated by the Intercept Layer for OpenCL™ Applications in the following ways:
* __Google* Chrome* trace event profiling tool__: JSON files generated by the Intercept Layer for OpenCL™ Applications contain device timeline information. You can open these JSON files in the [Google* Chrome* trace event profiling tool](chrome://tracing/) to generate a visual representation of the profiling data.
* __Microsoft* Excel*__: The Intercept Layer for OpenCL™ Applications contains a Python script that parses the timeline information into a Microsoft* Excel* file, where it is presented both in a table format and in a bar graph.

This tutorial will use the Google* Chrome trace event profiling tool for visualization.

Use the visualized data to identify gaps in the runtime where events are waiting for something else to finish executing. These gaps represent potential opportunities for system-level optimization. While it is not possible to eliminate all such gaps, you might be able to eliminate those caused by dependencies that can be avoided.

### Tutorial Example: Double Buffering

This tutorial is based on the *double-buffering* optimization. Double-buffering allows host data processing and host transfers to the device-side buffer to occur in parallel with the kernel execution on the FPGA device. This parallelization is useful when the host performs any combination of the following actions between consecutive kernel runs:
* Preprocessing
* Postprocessing
* Writes to the device buffer

By running host and device actions in parallel, execution gaps between kernels are removed as they no longer have to wait for the host to finish its operation. It's easy to see the benefits of double-buffering with the visualizations provided by the Intercept Layer output.

### Setting up the Intercept Layer for OpenCL™ Applications
The Intercept Layer for OpenCL™ Applications is available on GitHub at the following URL: <https://github.com/intel/opencl-intercept-layer>

To set up the Intercept Layer for OpenCL™ Applications, perform the following steps:

1) [Download](https://github.com/intel/opencl-intercept-layer) the Intercept Layer for OpenCL™ Applications version 2.2.1 or later from GitHub.


2) Build the Intercept Layer according to the instructions provided in [How to Build the Intercept Layer for OpenCL™ Applications](https://github.com/intel/opencl-intercept-layer/blob/master/docs/build.md).
     *  __Run `cmake`__: Ensure that you set `ENABLE_CLILOADER=1` when running cmake.
      (i.e. `cmake -DENABLE_CLILOADER=1 ..` )
    * __Run `make`__: After the cmake step, `make` must be run in the build directory. This step builds the `cliloader` loader utility.
    * __Add to your `PATH`__:    The `cliloader` executable should now exist in `<path to opencl-intercept-layer-master download>/<build dir>/cliloader/` directory. Add this directory to your `PATH` environment variable if you wish to run multiple designs using `cliloader`.

    You can now pass your executables to `cliloader` to run them with the intercept layer. For details about the `cliloader` loader utility, see [cliloader: A Intercept Layer for OpenCL™ Applications Loader](https://github.com/intel/opencl-intercept-layer/blob/master/docs/cliloader.md).

3) Set `cliloader` and other Intercept Layer options.

    If you run multiple designs with the same options, set up a `clintercept.conf` file in your home directory. You can also set the options as environment variables by prefixing the option name with `CLI_`. For example, the `DllName` option can be set through the `CLI_DllName` environment variable. For a list of options, see *Controls* in [How to Use the Intercept Layer for OpenCL™ Applications](https://github.com/intel/opencl-intercept-layer/blob/master/docs/controls.md).

    For this tutorial, set the following options:

| Options/Variables |Description
|:--- |:---
| `DllName=$CMPLR_ROOT/linux/lib/libOpenCL.so`                  | The intercept layer must know where `libOpenCL.so` file from the original oneAPI build is. If using this option in a conf file, expand `$CMPLR_ROOT` and pass in the absolute path.                           |
| `DevicePerformanceTiming=1` and `DevicePerformanceTimelineLogging=1`                    | These options print out runtime timeline information in the output of the executable run.                           |
| `ChromePerformanceTiming=1`, `ChromeCallLogging=1`, `ChromePerformanceTimingInStages=1` | These variables set up the chrome tracer output, and ensure the output has Queued, Submitted, and Execution stages. |


These instructions set up the `cliloader` executable, which provides some flexibility by allowing for more control over when the layer is used or not used. If you prefer a local installation (for a single design) or a global installation (always ON for all designs), follow the instructions at [How to Install the Intercept Layer for OpenCL™ Applications](https://github.com/intel/opencl-intercept-layer/blob/master/docs/install.md).

### Running the Intercept Layer for OpenCL™ Applications

To run a compiled program using the Intercept Layer for OpenCL™ Applications, use the command:
`cliloader <executable> [executable args]`

To run the tutorial example, refer to the "[Running the Sample](#running-the-sample)" section.

When you run the host executable with the `cliloader` command, the `stderr` output contains lines as shown in the following example:
```
Device Timeline for clEnqueueMapBuffer (enqueue 1) = 145810154237969 ns (queued), 145810156546459 ns (submit), 145810156552270 ns (start), 145810159109587 ns (end)
```

These lines give the timeline information about a variety of oneAPI runtime calls. After the host executable finishes running, there is also a summary of the run's performance information.

### Viewing the Performance Data

After the executable runs, the data collected will be placed in the `CLIntercept_Dump` directory, which is in the home directory by default. Its location can be adjusted using the `DumpDir=<directory where you want the output files>` `cliloader` option (if you are using the option inline, remember to use `CLI_DumpDir`). `CLIntercept_Dump` contains a folder named after the executable with a file called `clintercept_trace.json`. You can load this JSON file in the [Google* Chrome trace event profiling tool](chrome://tracing/) to visualize the timeline data collected by the run.

For this tutorial, this visualization appears as shown in the following example:

![](assets/full_example_trace.PNG)

This visualization shows different calls executed through time. The X-axis is time, with the scale shown near the top of the page. The Y-axis shows different calls that are split up in several ways.

The left side (Y-axis) has two different types of numbers:
* Numbers that contain a decimal point.
   * The part of the number before the decimal point orders the calls approximately by start time.
   * The part of the number after the decimal point represents the queue number the call was made from.
* Numbers that do not contain a decimal point. These numbers represent the thread ID of the thread being run on in the operating system.

The colors in the trace represent different stages of execution:
* Blue during the *queued* stage
* Yellow during the *submitted* stage
* Orange for the *execution* stage

Look for gaps between consecutive execution stages and kernel runs to identify possible areas for optimization.


### Applying Double-Buffering Using the Intercept Layer for OpenCL™ Applications

The double-buffering optimization can help minimize or remove gaps between consecutive kernels as they wait for host processing to finish. These gaps are minimized or removed by having the host perform processing operations on a second set of buffers while the kernel executes. With this execution order, the host processing is done by the time the next kernel can run, so kernel execution is not held up waiting for the host.

For a more detailed explanation of the optimization, refer to the FPGA tutorial "Double Buffering to Overlap Kernel Execution with Buffer Transfers and Host Processing".

In this tutorial, the first three kernels are run without the double-buffer optimization, and the next three are run with it. The kernels were run on an Arria® 10 FPGA when the intercept layer data was collected. The result of this optimization can be clearly seen in the Intercept Layer for OpenCL™ Applications trace:

![](assets/with_and_without_double_buffering.PNG)

Here, the kernel runs named `_ZTS10SimpleVpow` can be recognized as the bars with the largest execution time (the large orange bars). Double buffering removes the gaps between the kernel executions that can be seen in the top trace image. This optimization improves the throughput of the design, as explained in the `double_buffering` tutorial.

The Intercept Layer for OpenCL™ Applications makes it clear why the double buffering optimization will benefit this design and shows the performance improvement it achieves. Use the Intercept Layer tool on your designs to identify scenarios where you can apply double buffering and other system-level optimizations.

## Key Concepts
* A summary of the key profiling tools available for performance optimization
* Understanding the Intercept Layer for OpenCL™ Applications tool
* How to set up and use the Intercept Layer for OpenCL™ Applications tool
* How to use the resulting information to identify opportunities for system-level optimizations such as double buffering

## Building the Tutorial

> **Note**: When working with the command-line interface (CLI), you should configure the oneAPI toolkits using environment variables.
> Set up your CLI environment by sourcing the `setvars` script located in the root of your oneAPI installation every time you open a new terminal window.
> This practice ensures that your compiler, libraries, and tools are ready for development.
>
> Linux*:
> - For system wide installations: `. /opt/intel/oneapi/setvars.sh`
> - For private installations: ` . ~/intel/oneapi/setvars.sh`
> - For non-POSIX shells, like csh, use the following command: `bash -c 'source <install-dir>/setvars.sh ; exec csh'`
>
> Windows*:
> - `C:\"Program Files (x86)"\Intel\oneAPI\setvars.bat`
> - Windows PowerShell*, use the following command: `cmd.exe "/K" '"C:\Program Files (x86)\Intel\oneAPI\setvars.bat" && powershell'`
>
> For more information on configuring environment variables, see [Use the setvars Script with Linux* or macOS*](https://www.intel.com/content/www/us/en/develop/documentation/oneapi-programming-guide/top/oneapi-development-environment-setup/use-the-setvars-script-with-linux-or-macos.html) or [Use the setvars Script with Windows*](https://www.intel.com/content/www/us/en/develop/documentation/oneapi-programming-guide/top/oneapi-development-environment-setup/use-the-setvars-script-with-windows.html).

### On a Linux* System

1. Generate the `Makefile` by running `cmake`.
    ```
    mkdir build
    cd build
    ```
    To compile for the default target (the Agilex® 7 device family), run `cmake` using the command:
    ```
    cmake ..
    ```

    > **Note**: You can change the default target by using the command:
    >  ```
    >  cmake .. -DFPGA_DEVICE=<FPGA device family or FPGA part number>
    >  ```
    >
    > Alternatively, you can target an explicit FPGA board variant and BSP by using the following command:
    >  ```
    >  cmake .. -DFPGA_DEVICE=<board-support-package>:<board-variant>
    >  ```
  > **Note**: You can poll your system for available BSPs using the `aoc -list-boards` command. The board list that is printed out will be of the form
  > ```
  > $> aoc -list-boards
  > Board list:
  >   <board-variant>
  >      Board Package: <path/to/board/package>/board-support-package
  >   <board-variant2>
  >      Board Package: <path/to/board/package>/board-support-package
  > ```
    >
    > You will only be able to run an executable on the FPGA if you specified a BSP.

2. Compile the design through the generated `Makefile`. The following build targets are provided:

   * Compile for emulation (fast compile time, targets emulated FPGA device):
      ```
      make fpga_emu
      ```
   * Compile for simulation (fast compile time, targets simulated FPGA device, reduced data size):

     ```bash
     make fpga_sim
     ```
   * Compile for FPGA hardware (longer compile time, targets FPGA device):
     ```
     make fpga
     ```

## Running the Sample

 1. Run the sample on the FPGA emulator (the kernel executes on the CPU):
     ```
     ./double_buffering.fpga_emu     (Linux)
     ```
2. Run the sample on the FPGA simulator device:
     ```
     CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 ./double_buffering.fpga_sim     (Linux)
     ```
     > **Note**: for the following steps, you need to have access to an FPGA device, and the code must have been compiled by targeting its BSP.
3. Run the sample on the FPGA device (only if you ran `cmake` with `-DFPGA_DEVICE=<board-support-package>:<board-variant>`):
     ```
     ./double_buffering.fpga         (Linux)
     ```
4. Follow the instructions in the "[Setting up the Intercept Layer for OpenCL™ Applications](#setting-up-the-intercept-layer-for-opencl-applications)" section to install and configure the `cliloader` tool.
5. Run the sample using the Intercept Layer for OpenCL™ Applications to obtain system-level profiling information:
     ```
     cliloader ./double_buffering.fpga   (Linux)
     ```
6. Follow the instructions in the "[Viewing the Performance Data](#viewing-the-performance-data)" section to visualize the results.

### Example of Output
__Intercept Layer for OpenCL™ Applications results:__
Your visualization results should resemble the screenshots in sections "[Viewing the Performance Data](#viewing-the-performance-data)" and "[Applying Double-Buffering Using the Intercept Layer for OpenCL™ Applications](#applying-double-buffering-using-the-intercept-layer-for-opencl-applications)".

__Command line `stdout`:__
When run without `cliloader`, the tutorial output should resemble the result below.
```
Platform name: Intel(R) FPGA SDK for OpenCL(TM)
Running on device: ofs_n6001 : Intel OFS Platform (ofs_ee00000)
Executing kernel 3 times in each round.

*** Beginning execution, without double buffering
Launching kernel #0

Overall execution time without double buffering = 48 ms
Total kernel-only execution time without double buffering = 38 ms
Throughput = 64.228554 MB/s


*** Beginning execution, with double buffering.
Launching kernel #0

Overall execution time with double buffering = 40 ms
Total kernel-only execution time with double buffering = 38 ms
Throughput = 77.011658 MB/s


Verification PASSED
```

## License

Code samples are licensed under the MIT license. See [License.txt](/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
