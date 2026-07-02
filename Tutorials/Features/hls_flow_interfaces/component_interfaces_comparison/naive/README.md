# Naive Design

This simple vector addition implementation is similar to the one presented in the [fpga_compile](/Tutorials/GettingStarted/fpga_compile) code sample. It demonstrates a kernel that uses a register-mapped invocation interface as well as a register-mapped data interface. The compiler generates a single shared memory-mapped host data interface for the pointer kernel arguments and the arguments are stored in the register map. <!-- These are the default interfaces that are selected by the HLS IP Gen Compiler, and therefore this is the only design that can support the full-system compilation flow.-->

![](../assets/naive.svg)

## Interface Summary

<table>
  <tr>
    <th>Interface</th>
    <th>Type</th>
    <th>Variable</th>
  </tr>
  <tr>
    <td>Kernel invocation</td>
    <td rowspan="5">Avalon® MM agent (CSR)</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td rowspan="4">Kernel argument</td>
    <td><code>a_in</code> (pointer address)</td>
  </tr>
  <tr>
    <td><code>b_in</code> (pointer address)</td>
  </tr>
  <tr>
    <td><code>c_out</code> (pointer address)</td>
  </tr>
  <tr>
    <td><code>len</code></td>
  </tr>
  <tr>
    <td>External memory</td>
    <td>Avalon MM host (single shared)</td>
    <td>shared by <code>a_in</code>, <code>b_in</code>, <code>c_out</code> (data)</td>
  </tr>
</table>

## Register-Mapped Invocation Interface

By default, an un-decorated kernel will have all its control signals and arguments mapped into the IP component's control/status register (CSR).

## Register-Mapped Kernel Argument Interface

Unless otherwise customized, the data interfaces inherit the same style as the invocation interfaces. Since by default, the invocation interface is a register-mapped interface, data interfaces also default to register-mapped interfaces.

Hence, in this design, the pointer arguments `a_in`, `b_in`, `c_out` and scalar argument `len` are passed through the IP component's CSR. The IP component accesses the data pointed to by the pointers through a single memory-mapped host interface.

## Example Output

```
Add two vectors of size 256
PASSED
```

## License

Code samples are licensed under the MIT license. See
[License.txt](/License.txt) for details.

Third party program Licenses can be found here: [third-party-programs.txt](/third-party-programs.txt).
