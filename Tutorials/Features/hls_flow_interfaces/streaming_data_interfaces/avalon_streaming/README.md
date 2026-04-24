# Avalon® Streaming Interface

This design demonstrates host pipes configured with Avalon® streaming interfaces, with a data type of `StreamingBeat` to enable packet sideband signals on the design interface.

## Avalon® Streaming Sideband Signals

Pipes with Avalon® streaming protocol support a subset of Avalon® streaming sideband signals:

- `startofpacket` (`sop`): Asserted on the first beat of a packet, so the receiver knows where the packet begins.
- `endofpacket` (`eop`): Asserted on the last beat of a packet, so the receiver knows where the packet ends.
- `empty`: On the cycle where `eop` is asserted, indicates how many symbol positions in the data word are unused (padding) in that final beat.

You can add these to your Avalon® streaming pipe interface by using the special `StreamingBeat` structure provided by the `pipes_ext.hpp` header file (`sycl/ext/altera/experimental/pipes_ext.hpp`) as the `dataT` of your pipe. Only the `StreamingBeat` structure generates Avalon® sideband signals when used with a pipe.

The `StreamingBeat` structure is templated on three parameters, as summarized in Table 1.

#### Table 1. Template Parameters of the `StreamingBeat` Structure

| Template Parameter                      | Description
| ---                                     | ---
| `dataT`                                 | The datatype of elements carried by the `data` signal of the Avalon® streaming interface.
| `uses_packets`                          | A boolean that indicates whether to enable the `startofpacket` (`sop`) and `endofpacket` (`eop`) sideband signals on the Avalon® streaming interface.
| `uses_empty`                            | A boolean that indicates whether to enable the `empty` sideband signal on the Avalon® streaming interface.

#### Example 1.

The following example shows how to configure a `StreamingBeat` structure to use `sop`, `eop` and `empty` by setting the second and third template parameters to `true`. Using the `StreamingBeatT` type defined here in a pipe will result in `startofpacket`, `endofpacket`, and `empty` signals appearing on the resulting Avalon® streaming interface.

```c++
#include <sycl/ext/altera/fpga_extensions.hpp>
#include <sycl/ext/altera/experimental/pipes_ext.hpp>

namespace altera_exp = sycl::ext::altera::experimental;

...

using StreamingBeatDataT =altera_exp::StreamingBeat<unsigned short, true, true>;
using PipeInstance = altera_exp::pipe<class PipeID, StreamingBeatDataT>;

...

StreamingBeatT out_beat(data, sop, eop, empty);
PipeInstance::write(out_beat);
```

## Understanding the Tutorial

In `streaming_data_interfaces.cpp`, two pipes are declared to implement the input and output streaming interfaces on a kernel which thresholds pixel values in an image. The streams use `startofpacket` and `endofpacket` signals to determine the beginning and end of the image.

### Example Output

```
Running on device: <flow-dependent_device>

Writing 256 pixels to the Avalon input stream
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

After compiling in the simulation flow (`make fpga_sim`) and running the resulting executable, locate and run the `view_waveforms.sh` script in the `streaming_data_interfaces.fpga_sim.prj/` directory. Here you can see the `ready`, `valid` and `data` signals of the streaming input and streaming output interfaces (`InStream` and `OutStream` respectively). You can also see the `startofpacket` and `endofpacket` sideband signals that were added to the interfaces.

<p align="center">
  <img src=../assets/avst_waveform.png />
</p>

## License

Code samples are licensed under the MIT license. See [License.txt](/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
