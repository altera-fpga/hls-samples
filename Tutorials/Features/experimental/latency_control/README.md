# `Latency Control` Sample

This sample is an FPGA tutorial that demonstrates how to set latency constraints to pipes and load-store units (LSUs) accesses.

| Area                 | Description
|:--                   |:--
| What you will learn  | How to set latency constraints to pipes and LSUs accesses.<br>How to confirm that the compiler respected the latency control directive.
| Time to complete     | 15 minutes
| Category             | Concepts and Functionality

## Purpose

This FPGA tutorial demonstrates how to set latency constraints to pipes and LSUs accesses and how to confirm that the compiler respected the latency control directive.

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

> **Warning** Make sure you add the device files associated with the FPGA that you are targeting to your Quartus® Prime installation.

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

This sample demonstrates the following key concepts:

- Setting latency constraints to pipes and LSUs accesses.
- Confirming that the compiler respected the latency control directive.

### Use Model

> **Note**: The APIs described in this section are experimental. Future versions of latency controls might change these APIs in ways that are incompatible with the version described here.

Latency controls APIs are provided on member functions `read()` and `write()` of class `sycl::ext::altera::experimental::pipe` and on member functions `load()` and `store()` of class `sycl::ext::altera::experimental::lsu`. Other than the latency controls support, the `lsu` is identical to `sycl::ext::altera::lsu`. The experimental `pipe` and `lsu` are also provided by `<sycl/ext/altera/fpga_extensions.hpp>`.

These `read()`, `write()`, `load()`, and `store()` member functions can take a property list instance (`sycl::ext::oneapi::experimental::properties`) as a function argument, which can contain the following latency controls properties:

* `sycl::ext::altera::experimental::latency_anchor_id<N>`, where `N` is a signed integer

   A label that can be associated with the pipes and LSUs functions listed above. This label can then be referenced by the `latency_constraint` properties to define relative latency constraints. Functions with this property will be referred to as "labeled functions".

* `sycl::ext::altera::experimental::latency_constraint<A, B, C>`

    A constraint then can be associated with the pipes and LSUs functions listed above. It provides a latency constraint between this function and a different labeled function. Functions which have this property will be referred to as "constrained functions". This constraint has  the following parameters:

  * `A` is a signed integer: The label of the labeled function, which is constrained relative to the constrained function.
  * `B` is an enum value: The type of constraint, can be `latency_control_type::exact` (exact latency), `latency_control_type::max` (maximum latency), and `latency_control_type::min` (minimum latency).
  * `C` is a signed integer: The relative clock cycle difference between the labeled function and the constrained function, that the constraint should infer subject to the type of constraint (exact/max/main).

### Simple Code Example

The following example shows you how to use latency controls on pipes. The example also uses a function acting as both labeled function and constrained function:

```cpp
using Pipe1 = sycl::ext::altera::experimental::pipe<class PipeClass1, int, 8>;
using Pipe2 = sycl::ext::altera::experimental::pipe<class PipeClass2, int, 8>;
using Pipe3 = sycl::ext::altera::experimental::pipe<class PipeClass2, int, 8>;
...
// In kernel:
// The following read has a label 0.
int value = Pipe1::read(sycl::ext::oneapi::experimental::properties(
    sycl::ext::altera::experimental::latency_anchor_id<0>));

// The following write occurs exactly 2 cycles after the label-0 function, i.e.,
// the read above. Also, it has a label 1.
Pipe2::write(
    value,
    sycl::ext::oneapi::experimental::properties(
        sycl::ext::altera::experimental::latency_anchor_id<1>,
        sycl::ext::altera::experimental::latency_constraint<
            0, sycl::ext::altera::experimental::latency_control_type::exact,
            2>));

// The following write occurs at least 2 cycles after the label-1 function,
// i.e., the write above.
Pipe3::write(
    value,
    sycl::ext::oneapi::experimental::properties(
        sycl::ext::altera::experimental::latency_constraint<
            1, sycl::ext::altera::experimental::latency_control_type::min, 2>));
```

This next example shows you how to use latency controls on LSUs. It also uses a negative relative cycle number in `latency_constraint`, which means that the constrained function is scheduled __before__ the associated labeled function:

```cpp
using BurstCoalescedLSU = sycl::ext::altera::experimental::lsu<
    sycl::ext::altera::experimental::burst_coalesce<false>,
    sycl::ext::altera::experimental::statically_coalesce<false>>;
...
// In kernel:
// The following load occurs at most 5 cycles before the label-2 function,
// i.e., the store below.
int value = BurstCoalescedLSU::load(
    input_ptr,
    sycl::ext::oneapi::experimental::properties(
        sycl::ext::altera::experimental::latency_constraint<
            2, sycl::ext::altera::experimental::latency_control_type::max, -5>));

// The following store has a label 2.
BurstCoalescedLSU::store(
    output_ptr, value,
    sycl::ext::oneapi::experimental::properties(
        sycl::ext::altera::experimental::latency_anchor_id<2>));
```

### Rules and Limitations

* `latency_anchor_id` must be non-negative
* `latency_anchor_id` must be a unique number within the whole design
* The labeled function and the constrained function of a constraint must meet one of the following conditions:
  * Both are in the same block but not in any cluster
  * Both are in the same cluster

> **Note**: Clusters can be identified in the System Viewer (__Views > System Viewer__) of the `report.html` report.

The compiler tries to achieve the latency constraints, and it errors out if some constraints cannot be satisfied. For example, if one constraint specifies function A should be scheduled after function B, while another constraint specifies function B should be scheduled after function A, then that set of constraints is unsatisfiable.

## Build the `Latency Control` Tutorial

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

1. Locate `report.html` in the `latency_control.report.prj/reports/` directory.

2. Navigate to Schedule Viewer (__Views > Schedule Viewer__).

   In this view, you can find the load and the store that have latency constraint and then you can check the scheduled latency between them. The scheduled latency should reflect the applied latency control in the design source code. You can also verify the latency control parameters in the __Details__ pane for the load and store nodes.

3. Navigate to System Viewer (__Views > System Viewer__). 

   In this view, you can verify the latency control parameters in the __Details__ pane for the load and the store nodes. You can find the load and store nodes in __LatencyControl.B1__.

## Run the `Latency Control` Sample

## On Linux

1. Run the sample on the FPGA emulator (the kernel executes on the CPU).
   ```
   ./latency_control.fpga_emu
   ```

2. Run the sample on the FPGA simulator device.
   ```
   CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 ./latency_control.fpga_sim
   ```
### Example Output

```
PASSED: all kernel results are correct.
```

## License

Code samples are licensed under the MIT license. See [License.txt](/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
