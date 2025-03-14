# Global Memory Channels
This FPGA tutorial demonstrates how to use the `mem_channel` buffer property in
conjunction with the `-Xsno-interleaving` flag to reduce the area consumed by a
SYCL*-compliant FPGA design.


| Optimized for                     | Description
|:---                               |:---
| OS                                | Ubuntu* 20.04 <br> RHEL*/CentOS* 8 <br> SUSE* 15 <br> Windows* 10, 11 <br> Windows Server* 2019
| Hardware                          | Intel® Agilex® 7, Agilex® 5, Arria® 10, and Stratix® 10 FPGAs
| Software                          | Intel® oneAPI DPC++/C++ Compiler
| What you will learn               | How and when to use the `mem_channel` buffer property and the `-Xsno-interleaving` flag
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
You can also find more information about [troubleshooting build errors](/README.md#troubleshooting), [links to selected documentation](/README.md#documentation), etc.

## Purpose

This FPGA tutorial demonstrates an example of using the `mem_channel` buffer
property in conjunction with the `-Xsno-interleaving` flag to reduce the amount
of resources required to implement a SYCL-compliant FPGA design.

By default, the compiler configures each global memory type
in a burst-interleaved manner where memory words are interleaved across the
available memory channels. This usually leads to better throughput because it
prevents load imbalance by ensuring that memory accesses do not favor one
external memory channel over another. Moreover, burst-interleaving enables you
to view a multi-channel memory as one large channel so you can write your code
without worrying about where each buffer should be allocated. However, this
configuration can be expensive in terms of FPGA resources because the global
memory interconnect required to orchestrate the memory accesses across all the
channels is complex. For more information about burst-interleaving, please
refer to the [FPGA Optimization Guide for Intel® oneAPI Toolkits Developer Guide](https://software.intel.com/content/www/us/en/develop/documentation/oneapi-fpga-optimization-guide).

The compiler allows you to avoid this area overhead by
disabling burst-interleaving and assigning buffers to individual channels. There
are two advantages to such configuration:

1. A simpler global memory interconnect is built, which requires a smaller
   amount of FPGA resources than the interconnect needed for the
   burst-interleaving configuration.

2. Potential improvements to the global memory bandwidth utilization due to
   less contention at each memory channel.

Burst-interleaving should only be disabled in situations where satisfactory
load balancing can be achieved by assigning buffers to individual channels.
Otherwise, the global memory bandwidth utilization may be reduced, which will
negatively impact the throughput of your design.

To disable burst-interleaving, you need to pass the
`-Xsno-interleaving=<global_memory_type>` flag to your `icpx` command. In the 
case of targeting a BSP, the global memory type is indicated in the board 
specification XML file for the Board Support Package (BSP) that you're using. 
The board specification XML file, called `board_spec.xml`, can be found in the 
root directory of your BSP.
Note that this BSP only has a single memory type available as indicated in its
`board_spec.xml` file: `<global_mem name="DDR"`. The appropriate flag to pass
in this case is `-Xsno-interleaving=DDR`. Another option would be
`-Xsno-interleaving=default` since there is only one global memory type
available.

With interleaving disabled, you now have to specify, using the `mem_channel`
property, in which memory channel each buffer should be allocated:

```c++
buffer a_buf(a_vec, {property::buffer::mem_channel{1}});
buffer b_buf(b_vec, {property::buffer::mem_channel{2}});
```
Channel IDs are in the range `1,2,...,N` where `N` is the number of available
channels.

When `-Xsno-interleaving` is used,
buffers that don't have the `mem_channel` property will all be allocated in
channel `1`.

An important observation is that the `mem_channel` property is a runtime
specification which means you can change the selected channel ID for each
buffer without having to recompile the device code. In other words, when you
change the channel IDs, you only need to recompile the host code since
`mem_channel` has no impact on the RTL generated by the compiler. To learn how
to recompile the host code only and re-use an existing FPGA device image, please
refer to the FPGA tutorial "[Separating Host and Device Code Compilation](
/Tutorials/GettingStarted/fast_recompile)".

### Tutorial Design
The basic function performed by the tutorial kernel is an addition of 3
vectors. When burst-interleaving is disabled, each buffer is assigned to a
specific memory channel depending on how many channels are available.

Due to the nature of this design and the fact that all the buffers have the
same size, we expect accesses to the different memory channels to be well
balanced. Therefore, we can predict that disabling burst-interleaving will not
impact the throughput of the design, and so it is likely beneficial to disable
interleaving to avoid the area overhead imposed by the interleaving logic.

This tutorial requires compiling the source code twice: once with the
`-Xsno-interleaving` flag and once without it. In the `CMakeLists.txt` file,
the macro `NO_INTERLEAVING` is defined when the `-Xsno-interleaving` flag is
passed to the `icpx` command. The macro controls whether the buffers are
created with our without the `mem_channel` property.

To decide what channel IDs to select in the source code, the macros
`TWO_CHANNELS` and `FOUR_CHANNELS` are also used. The macro `TWO_CHANNELS` is
defined when the design is compiled for an Intel® Arria® GX FPGA. 
In that case, the 4 buffers are evenly assigned to the available 
channels on that board. When the design is compiled for an Intel® Stratix® or
Agilex® FPGA, the 4 buffers are assigned to the 4 available channels.
This can be parametrize by setting the correct macro (or create your
own) that clearly matches the number of channels available on your specific
board.

## Key Concepts
* How to disable global memory burst-interleaving using the
  `-Xsno-interleaving` flag and the `mem_channel` buffer property.
* The scenarios in which disabling burst-interleaving can help reduce the area
  consumed by a FPGA design without impacting throughput.

## Building the `mem_channel` Tutorial

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
  To compile for the default target (the Agilex® device family), run `cmake` using the command:
  ```
  cmake .. -DPART=<X>
  ```
   where `-DPART=<X>` is:
   - `-DPART=INTERLEAVING`
   - `-DPART=NO_INTERLEAVING`

  > **Note**: You can change the default target by using the command:
  >  ```
  >  cmake .. -DFPGA_DEVICE=<FPGA device family or FPGA part number>
  >  ``` 
  >
  > Alternatively, you can target an explicit FPGA board variant and BSP by using the following command: 
  >  ```
  >  cmake .. -DFPGA_DEVICE=<board-support-package>:<board-variant>
  >  ``` 
  > The build system will try to infer the FPGA family from the BSP name.
  > If it can't, an extra option needs to be passed to `cmake`: `-DDEVICE_FLAG=[A10|S10|CycloneV|Agilex5|Agilex7]` 
  > **Note**: You **must** set `FPGA_DEVICE` to point to a BSP in order to build this sample. You can poll your system for available BSPs using the `aoc -list-boards` command. The board list that is printed out will be of the form
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

2. Compile the design through the generated `Makefile`. The following build
   targets are provided, matching the recommended development flow:

   * Compile for emulation (fast compile time, targets emulated FPGA device):
      ```
      make fpga_emu
      ```
   * Generate the optimization report:
     ```
     make report
     ```
   * Compile for simulation (fast compile time, targets simulated FPGA device, reduced data size):
     ```
     make fpga_sim
     ```
   * Compile for FPGA hardware (longer compile time, targets FPGA device):
     ```
     make fpga
     ```

### On a Windows* System

1. Generate the `Makefile` by running `cmake`.
  ```
  mkdir build
  cd build
  ```
  To compile for the default target (the Agilex® device family), run `cmake` using the command:
  ```
  cmake -G "NMake Makefiles" .. -DPART=<X>
  ```
   where `-DPART=<X>` is:
   - `-DPART=INTERLEAVING`
   - `-DPART=NO_INTERLEAVING`

  > **Note**: You can change the default target by using the command:
  >  ```
  >  cmake -G "NMake Makefiles" .. -DFPGA_DEVICE=<FPGA device family or FPGA part number>
  >  ``` 
  >
  > Alternatively, you can target an explicit FPGA board variant and BSP by using the following command: 
  >  ```
  >  cmake -G "NMake Makefiles" .. -DFPGA_DEVICE=<board-support-package>:<board-variant>
  >  ``` 
  > The build system will try to infer the FPGA family from the BSP name.
  > If it can't, an extra option needs to be passed to `cmake`: `-DDEVICE_FLAG=[A10|S10|CycloneV|Agilex5|Agilex7]` 
  > **Note**: You **must** set `FPGA_DEVICE` to point to a BSP in order to build this sample.You can poll your system for available BSPs using the `aoc -list-boards` command. The board list that is printed out will be of the form
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

2. Compile the design through the generated `Makefile`. The following build
   targets are provided, matching the recommended development flow:

   * Compile for emulation (fast compile time, targets emulated FPGA device):
     ```
     nmake fpga_emu
     ```
   * Generate the optimization report:
     ```
     nmake report
     ```
   * Compile for simulation (fast compile time, targets simulated FPGA device, reduced data size):
     ```
     nmake fpga_sim
     ```
   * Compile for FPGA hardware (longer compile time, targets FPGA device):
     ```
     nmake fpga
     ```

> **Note**: If you encounter any issues with long paths when compiling under
Windows*, you may have to create your 'build' directory in a shorter path, for
example c:\samples\build.  You can then run cmake from that directory, and
provide cmake with the full path to your sample directory, for example:
>
>  ```
  > C:\samples\build> cmake -G "NMake Makefiles" C:\long\path\to\code\sample\CMakeLists.txt
>  ```
## Examining the Reports
After generating the reports of both parts of the sample, locate the `report.html` files in the `mem_channel.report.prj` directories. Open the reports in 
Chrome*, Firefox*, Edge*, or Internet Explorer*. In the "Summary" tab, locate
the "Quartus Fitter Resource Utilization Summary" entry and expand it to see
the table showing the FPGA resources that were allocated for the design. Notice
that when burst-interleaving is disabled, the FPGA resources required are
significantly lower than the case where burst-interleaving is enabled.


## Running the Sample

 1. Run the sample on the FPGA emulator (the kernel executes on the CPU):
     ```
     ./mem_channel.fpga_emu     (Linux)
     mem_channel.fpga_emu.exe   (Windows)
     ```
    Note that the `mem_channel` property and the `-Xsno-interleaving` flag have
    no impact on the emulator which is why we only have a single executable for
    this flow.
2. Run the sample on the FPGA simulator device (the kernel executes on the CPU):
  * On Linux
    ```bash
    CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 ./mem_channel.fpga_sim
    ```
  * On Windows
    ```bash
    set CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1
    mem_channel.fpga_sim.exe
    set CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=
    ```
    Note that the `mem_channel` property and the `-Xsno-interleaving` flag have
    no impact on the simulator which is why we only have a single executable for
    this flow.

> **Note**: Hardware runs are not supported on Windows.

### Example of Output

Running `./mem_channel.fpga` when compiled with `-DPART=INTERLEAVING`:
```
Running on device: ofs_n6001 : Intel OFS Platform (ofs_ec00000)

Vector size: 1000000
Verification PASSED

Kernel execution time: 0.001674 seconds
Kernel throughput: 1791.987308 MB/s
```

Running `./mem_channel.fpga` when compiled with `-DPART=NO_INTERLEAVING`:
```
Running on device: ofs_n6001 : Intel OFS Platform (ofs_ec00000)

Vector size: 1000000
Verification PASSED

Kernel execution time: 0.001673 seconds
Kernel throughput without burst-interleaving: 1793.003700 MB/s
```

### Discussion of Results

A test compile of this tutorial design achieved the following results on the Intel® FPGA SmartNIC N6001-PL. 
The table below shows the performance of the design as well as the resources consumed by
the kernel system.
Configuration | Execution Time (ms) | Throughput (MB/s) | ALM | REG | MLAB | RAM | DSP
|:--- |:--- |:--- |:--- |:--- |:--- |:--- |:--- 
|Without `-Xsno-interleaving` | 1.674 | 1791 | 3192 | 14198 | 6  | 345 | 0 
|With `-Xsno-interleaving` | 1.673 | 1793 | 1997  | 12458  | 6 | 186  | 0

Notice that the throughput of the design when burst-interleaving is disabled is
equal or better than when burst-interleaving is enabled. However, the resource
utilization is significantly lower without burst-interleaving. Therefore, this
is a design where disabling burst-interleaving and manually assigning buffers
to channels is a net win.

## License

Code samples are licensed under the MIT license. See [License.txt](/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
