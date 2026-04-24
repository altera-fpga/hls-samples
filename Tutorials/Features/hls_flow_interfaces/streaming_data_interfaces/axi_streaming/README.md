# AXI™ Streaming Interface

This design demonstrates host pipes configured with AXI™ streaming interfaces, with a data type of `StreamingBeatAxi` to enable packet sideband signals on the design interface.

To use an AXI™ streaming interface, explicitly set the pipe `protocol` property to `protocol_axi_streaming`.

## AXI™ Streaming Sideband Signals

Pipes with AXI™ streaming protocol support a subset of AXI™ streaming sideband signals:

- `tlast`: Asserted on the last beat of a packet, so the receiver knows where the packet ends. Equivalent to Avalon® streaming `endofpacket` signal.
- `tuser`: Carries per-beat sideband metadata you define; the bit width matches the C++ type you supply when `uses_tuser` is `true`.

You can add these sideband signals to your AXI™ streaming pipe interface by using the special `StreamingBeatAxi` structure provided by the `pipes_ext.hpp` header file (`sycl/ext/altera/experimental/pipes_ext.hpp`) as the `dataT` of your pipe. Only `StreamingBeatAxi` generates these AXI™ sideband signals when used with a pipe.

The `StreamingBeatAxi` structure is templated on four parameters, as summarized in Table 1.

#### Table 1. Template Parameters of the `StreamingBeatAxi` Structure

| Template Parameter                      | Description
| ---                                     | ---
| `tdataT`                                | The datatype of elements carried on the `tdata` signal of the AXI™ streaming interface.
| `tuserT`                                | The datatype of the `tuser` signal when `uses_tuser` is `true`. Omit when `uses_tuser` is `false`.
| `uses_tlast`                            | A boolean that indicates whether to enable the `tlast` sideband signal on the AXI™ streaming interface.
| `uses_tuser`                            | A boolean that indicates whether to enable the `tuser` sideband signal on the AXI™ streaming interface.

#### Example 1.

The following example shows how to configure a `StreamingBeat` structure to use `tlast` and `tuser` by setting the appropriate template parameters. Using the `StreamingBeatAxiT` type defined here in a pipe with `protocol_axi_streaming` property will result in `tlast` and `tuser` signals appearing on the resulting AXI™ streaming interface.

```c++
#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/ext/altera/experimental/pipes_ext.hpp>

namespace altera_exp = sycl::ext::altera::experimental;
namespace oneapi_exp = sycl::ext::oneapi::experimental;

...

using AxiPipePropertiesT = decltype(oneapi_exp::properties(
    altera_exp::ready_latency<0>,
    altera_exp::bits_per_symbol<8>,
    altera_exp::uses_valid<true>,
    altera_exp::uses_ready<true>,
    altera_exp::first_symbol_in_high_order_bits<false>,
    altera_exp::protocol_axi_streaming));
using AxiBeatDataT =
    altera_exp::StreamingBeatAxi<unsigned long long, unsigned char, true, true>;
using PipeInstance =
    altera_exp::pipe<class PipeID, AxiBeatDataT, 0, AxiPipePropertiesT>;

...

AxiBeatDataT out_beat(data, tlast, tuser);
PipeInstance::write(out_beat);
```

## Understanding the Tutorial

In `streaming_data_interfaces.cpp`, two pipes are declared to implement the input and output streaming interfaces on a kernel which thresholds pixel values in an image. The streams use the `tlast` sideband to mark the last beat of the image (this sample does not enable `tuser`).

### Example Output

```
Running on device: <flow-dependent_device>

Writing 256 pixels to the AXI input stream
Launching the kernel
Checking that output pixels are below the threshold

PASSED
```

### Reading the Reports

After compiling the `report` target, locate and open the `report.html` file in the `streaming_data_interfaces.report.prj/reports/` directory. Under the `Threshold` kernel in the System Viewer, the streaming in and streaming out interfaces can be seen, shown by the pipe read and pipe write nodes respectively. Clicking on either of these nodes gives further information about these interfaces in the 'details' pane. The 'details' pane will identify that the read is coming from `InStream`, and that the write is going to `OutStream`, as well as verifying that both interfaces have a width of 32 bits (corresponding to size of the `StreamingBeatT` type) and depth of 0 (which is the capacity that each pipe was declared with).

<p align="center">
  <img src=../assets/kernel.png />
</p>

### Viewing the Simulation Waveform

After compiling in the simulation flow (`make fpga_sim`) and running the resulting executable, locate and run the `view_waveforms.sh` script in the `streaming_data_interfaces.fpga_sim.prj/` directory. Here you can see the `tready`, `tvalid` and `tdata` signals of the streaming input and streaming output interfaces (`InStream` and `OutStream` respectively). You can also see the `tlast` sideband signal that was added to the interfaces.

<p align="center">
  <img src=../assets/axist_waveforms.png />
</p>

## License

Code samples are licensed under the MIT license. See [License.txt](/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
