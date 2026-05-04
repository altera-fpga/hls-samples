# `FPGA Template` Sample

This project serves as a template for HLS FPGA designs, and demonstrates the features of the base CMake build system in our code samples.

| Optimized for                     | Description
|:---                               |:---
| OS                                | Ubuntu* 20.04, Ubuntu* 22.04, Ubuntu* 24.04 <br> RHEL* 8, RHEL* 9 <br> SUSE* 15 <br> **NOTE: Windows is not supported**
| Hardware                          | Agilex® 3, Agilex® 5, Agilex® 7, Stratix® 10 and Arria® 10 FPGAs
| Software                          | HLS IP Gen Compiler
| What you will learn               | Best practices for creating and managing a oneAPI FPGA project
| Time to complete                  | 10 minutes

> **Note**: Even though the HLS IP Gen compiler is enough to compile for emulation, generating reports and generating RTL, there are extra software requirements for the simulation flow and FPGA compiles.
>
> To use the simulator flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) and one of the following simulators must be installed and accessible through your PATH:
> - Questa*-Intel® FPGA Edition
> - Questa*-Intel® FPGA Starter Edition
> - ModelSim® SE
>
> When using the hardware compile flow, Quartus® Prime Pro Edition (or Standard Edition when targeting Cyclone® V) must be installed and accessible through your PATH.
>
> :warning: Make sure you add the device files associated with the FPGA that you are targeting to your Quartus® Prime installation.

## Prerequisites

This sample is part of the FPGA code samples.
It is categorized as a Tier 1 sample that helps you getting started.

```mermaid
flowchart LR
   tier1("Tier 1: Get Started")
   tier2("Tier 2: Explore the Fundamentals")
   tier3("Tier 3: Explore the Advanced Techniques")
   tier4("Tier 4: Explore the Reference Designs")

   tier1 --> tier2 --> tier3 --> tier4

   style tier1 fill:#f96,stroke:#333,stroke-width:1px,color:#fff
   style tier2 fill:#0071c1,stroke:#0071c1,stroke-width:1px,color:#fff
   style tier3 fill:#0071c1,stroke:#0071c1,stroke-width:1px,color:#fff
   style tier4 fill:#0071c1,stroke:#0071c1,stroke-width:1px,color:#fff
```

Find more information about how to navigate this part of the code samples in the [FPGA top-level README.md](/README.md).
You can also find more information about [troubleshooting build errors](/README.md#troubleshooting), [links to selected documentation](/README.md#documentation), etc.

## Purpose

Use this project as a starting point when you build designs for the HLS IP Gen compiler when targeting FPGAs. It includes a CMake build system to automate selecting the various command-line flags for the HLS IP Gen compiler, and a simple single-source design to serve as an example. You can customize the build flags by modifying the top part of `CMakeLists.txt`: if you want to pass additional flags to the HLS IP Gen compiler, you can change the `USER_FLAGS` and `USER_FPGA_FLAGS` variables defined in `CMakeLists.txt`. Similarly, you can add additional include paths to the `USER_INCLUDE_PATHS` variable. You can also explicitly define these variables at the command-line if you want to perform a quick test without changing the CMake build system.

> **Note**: The code sample in this design uses USM shared allocations for improved code simplicity as compared with buffers/accessors. The included CMake build system can also be used for designs that use buffers and accessors or USM device allocations.

| Variable              | Description
|:---                   |:---
| `USER_FPGA_FLAGS` | This semicolon-separated list of flags will be applied **only** to flows that generate FPGA hardware (namely report, simulation, hardware). You can specify flags such as `-Xsclock` or `-Xshyper-optimized-handshaking=off`
| `USER_FLAGS`          | This semicolon-separated list of flags applies to **all** flows, including emulation. You can specify flags such as `-v` or define macros such as `-DYOUR_OWN_MACRO=3`
| `USER_INCLUDE_PATHS`  | This semicolon-separated list of include paths applies to all flows, including emulation. Specify include paths relative to the `CMakeLists.txt` file, or using absolute paths in the filesystem.

```bash
###############################################################################
### Customize these build variables
###############################################################################
set(SOURCE_FILES src/fpga_template.cpp)
set(TARGET_NAME fpga_template)

# Use cmake -DFPGA_DEVICE=<board-support-package>:<board-variant> to choose a
# different device. 
#
# You can also specify a device family (E.g. "Arria10" or "Stratix10") or a
# specific part number (E.g. "10AS066N3F40E2SG") to generate a standalone IP.
if(NOT DEFINED FPGA_DEVICE)
    set(FPGA_DEVICE "Agilex7")
endif()

# Use cmake -DUSER_FPGA_FLAGS=<flags> to set extra flags for FPGA backend
# compilation. 
set(USER_FPGA_FLAGS ${USER_FPGA_FLAGS})

# Use cmake -DUSER_FLAGS=<flags> to set extra flags for general compilation.
set(USER_FLAGS ${USER_FLAGS})

# Use cmake -DUSER_INCLUDE_PATHS=<paths> to set extra paths for general
# compilation.
set(USER_INCLUDE_PATHS ../../../include;${USER_INCLUDE_PATHS})
```

Everything below this in the `CMakeLists.txt` is necessary for selecting the compiler flags that are necessary to support the build targets specified below, and should not need to be modified.

## Building the `fpga_template` Tutorial

> **Note**: When working with the command-line interface (CLI), you should configure the HLS IP Gen Compiler using environment variables.
> Set up your CLI environment by sourcing the `fpgavars` script located in the root of your HLS IP Gen Compiler installation every time you open a new terminal window.
> This practice ensures that your compiler, libraries, and tools are ready for development.
>
> Linux*:
> - `source <install-dir>/fpgavars.sh`
> - For non-POSIX shells, like csh, use the following command: `bash -c 'source <install-dir>/fpgavars.sh ; exec csh'`

Use these commands to run the design.

### On a Linux* System
This design uses CMake to generate a build script for GNU/make.

1. Change to the sample directory.

2. Configure the build system for the Agilex® 7 device family, which is the default.

   ```
   mkdir build
   cd build
   cmake ..
   ```

   You can create a debuggable binary by setting `CMAKE_BUILD_TYPE` to `Debug`:

   ```
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   ```

   If you want to use the `report`, `fpga_sim`, or `fpga` flows, you should switch the `CMAKE_BUILD_TYPE` back to `Release`:

   ```
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

   > **Note**: You can change the default target by using the following command. **Targeting a BSP is not supported.**
   >  ```
   >  cmake .. -DFPGA_DEVICE=<FPGA device family or FPGA part number>
   >  ```
 
3. Compile the design with the generated `Makefile`. The following build targets are provided, matching the recommended development flow:

   | Target          | Expected Time  | Output                                                                       | Description
   |:---             |:---            |:---                                                                          |:---
   | `make fpga_emu` | Seconds        | x86-64 binary                                                                | Compiles the FPGA device code to the CPU. Use the Intel® FPGA Emulation Platform for OpenCL™ software to verify your SYCL code’s functional correctness.
   | `make report`   | Minutes        | RTL + FPGA reports                                                           | Compiles the FPGA device code to RTL and generates an optimization report that describes the structures generated on the FPGA, identifies performance bottlenecks, and estimates resource utilization. The generated RTL may be exported to Quartus Prime software.
   | `make fpga_sim` | Minutes        | RTL + FPGA reports + x86-64 binary                                           | Compiles the FPGA device code to RTL and generates a simulation testbench. Use the Questa*-Intel® FPGA Edition simulator to verify your design.
   | `make fpga`     | Multiple Hours | Quartus Place & Route + FPGA reports | Compiles the FPGA device code to RTL and compiles the generated RTL using Quartus® Prime.

   The `fpga_emu` and `fpga_sim` targets produce binaries that you can run. The executables will be called `TARGET_NAME.fpga_emu`, and `TARGET_NAME.fpga_sim`, where `TARGET_NAME` is the value you specify in `CMakeLists.txt`.

   You can see a listing of the commands that are run:

   ```bash
   build $> make report
   [ 33%] To compile manually:
   ahls -I../../../../include -fintelfpga -Wall -qactypes -DFPGA_HARDWARE -c ../src/fpga_template.cpp -o CMakeFiles/report.dir/src/ fpga_template.cpp.o

   To link manually:
   ahls -fintelfpga -Xshardware  -Xstarget=Agilex7 -fsycl-link=early -o fpga_template.report CMakeFiles/report.dir/src/  fpga_template.cpp.o
   
   [ ... ]

   [100%] Built target report
   ```

## Run the `fpga_template` Executable

### On Linux
1. Run the sample on the FPGA emulator (the kernel executes on the CPU).
   ```
   ./fpga_template.fpga_emu
   ```
2. Run the sample on the FPGA simulator device.
   ```
   CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 ./fpga_template.fpga_sim
  ```

## Example Output

```
Running on device: Intel(R) FPGA Emulation Device
add two vectors of size 256
PASSED
```
## License
Code samples are licensed under the MIT license. See
[License.txt](/License.txt) for details.

Third party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
