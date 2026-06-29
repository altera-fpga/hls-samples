# `Read-Only Cache` Sample

This sample is an FPGA tutorial that demonstrates how to use the read-only cache feature to boost
the throughput of a SYCL*-compliant FPGA design that requires reading from off-chip
memory in a non-contiguous manner.

| Area                 | Description
|:---                  |:---
| What you will learn  | How and when to use the read-only cache feature
| Time to complete     | 30 minutes
| Category             | Concepts and Functionality

## Purpose

This FPGA tutorial is an example of how to use read-only cache to
boost the throughput of an FPGA design. Specifically, the read-only cache
is most appropriate for non-contiguous read accesses from off-chip memory
buffers that are guaranteed to be constant throughout the execution of a
kernel. The read-only cache is optimized for high cache hit performance.

To enable the read-only cache, the `-Xsread-only-cache-size<N>` flag should be
passed to the `ahls` command. Each kernel will get its own *private* version
of the cache that serves all reads in the kernel from read-only no-alias
accessors. Read-only no-alias accessors are accessors that have both the
`read_only` and the `no_alias` properties:

```c++
accessor sqrt_lut(sqrt_lut_buf, h, read_only, accessor_property_list{no_alias});
```

The `read_only` property is required because the cache is **read-only**. The
`no_alias` property is required to guarantee that the buffer will not be
written to from the kernel through another accessor or through a USM pointer.
You do not need to apply the `no_alias` property if the kernel already has the
`[[intel::kernel_args_restrict]]` attribute.

Each private cache is also replicated as many times as needed so that it can
expose extra read ports. The size of each replicate is `N` bytes as specified
by the `-Xsread-only-cache-size=<N>` flag.

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

- How to use the read-only cache feature.
- Understanding scenarios in which this feature can help improve the throughput of a
  design.

### Tutorial Design

The basic function performed by the tutorial kernel is a series of table
lookups from a buffer (`sqrt_lut_buf`) that contains the square root values of
the first 512 integers. By default, the compiler will generate load-store units
(LSUs) optimized for the case where global memory accesses are
contiguous. When the memory accesses are non-contiguous, as is the case in this
tutorial design, these LSUs tend to suffer major throughput loss. The read-only
cache can sometimes help in such situations, especially when sized correctly.

This tutorial requires compiling the source code twice: once with the
`-Xsread-only-cache-size=<N>` flag and once without it. Because the look-up
table contains 512 integers as indicated by the `kLUTSize` constant, the chosen
size of the cache is `512*4 bytes = 2048 bytes`, and so, the flag
`-Xsread-only-cache-size=2048` is passed to `ahls`.

## Build the `Read-Only Cache` Tutorial

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
   - `CACHE_ENABLED`
   - `CACHE_DISABLED`
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

Locate the `report.html` files of each build (with and without the cache enabled).

Navigate to the "Area Analysis of System" section of each report
(Area Analysis > Area Analysis of System) and expand the "Kernel System" entry
in the table. Notice that when the read-only cache is enabled, a new entry
shows up in the table called "Constant cache interconnect" indicating that the
cache has been created.

## Run the `Read-Only Cache` Sample

### On Linux

 1. Run the sample on the FPGA emulator (the kernel executes on the CPU).
    ```
    ./read_only_cache.fpga_emu
    ```
    > **Note**: The read-only cache is not implemented in emulation. The `-Xsread-only-cache-size<N>` flag does not impact the emulator in any way, which is why we only have a single executable for this flow.

2. Run the sample on the FPGA simulation device (two executables should be generated).
   ```
   CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 ./read_only_cache.fpga_sim
   ```
   > **Note**: Although the circuit for the read-only cache is implemented in simulation, one cannot see consistent performance increases with the cache enabled, as each clock cycle in the simulator does not have a consistent latency as it does in the hardware. For this reason here is just a single executable for this flow.

## Example Output

### Example Output for `./read_only_cache.fpga` with `-DPART=CACHE_DISABLED`

```
Running on device: ofs_n6001 : Intel OFS Platform (ofs_ee00000)

SQRT LUT size: 512
Number of outputs: 131072
Verification PASSED

Kernel execution time: 0.006714 seconds
Kernel throughput: 74.469202 MB/s
```

### Example Output for `./read_only_cache.fpga` with `-DPART=CACHE_ENABLED`

```
Running on device: ofs_n6001 : Intel OFS Platform (ofs_ee00000)

SQRT LUT size: 512
Number of outputs: 131072
Verification PASSED

Kernel execution time: 0.000860 seconds
Kernel throughput with the read-only cache: 581.580643 MB/s
```

A test compile of this tutorial design achieved the following results on the Intel® FPGA SmartNIC N6001-PL:

|Configuration    | Execution Time (ms) | Throughput (MB/s)
|:---             |:---                 |:---
|Without caching  | 6.714               | 74.46
|With caching     | 0.860               | 581.58

When the read-only cache is enabled, performance notably increases. As
previously mentioned, when the global memory accesses are random (for example, non-contiguous), enabling the read-only cache and sizing it correctly may allow
the design to achieve a higher throughput.

## License

Code samples are licensed under the MIT license. See [License.txt](/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
