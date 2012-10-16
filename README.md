# YASAB
## Yet Another Simple AVR Bootloader

This AVR Bootloader just gets data from the included uploader. This will be flashed and started.
Currently supported AVR MCUs:
> ATmega32

It is using my [avrSerial Library](https://github.com/xythobuz/avrSerial) for UART communications.

Running "make" will produce yasab.hex and sample.hex. The first is the real bootloader. The second is a small program to test the bootloader.

Configuration Options like Baudrate, Bootdelay or Clock Frequency can be changed in the makefile.

## Uploader

This is a small utility to upload hex files to YASAB.
The optional parameter, q, can be a character that is sent to reset the target.
> Usage: uploader /dev/port /path/to.hex [q]

## Protocol

38400, 8N1, XON-XOFF will be used for data transmission.

<table>
<tr><th>Computer</th><th>AVR</th></tr>
<tr><td>Repeat 'f'</td><td></td></tr>
<tr><td></td><td>Respond with 'o'</td></tr>
<tr><td>Send 32bit target address (MSB first)</td><td></td></tr>
<tr><td></td><td>Respond with 'o' or 'e'</td></tr>
<tr><td>Send 32bit data length (MSB first)</td><td></td></tr>
<tr><td></td><td>Respond with 'o' or 'e'</td></tr>
<tr><td>Send data...</td><td></td></tr>
<tr><td></td><td>In between 'e' or 'o' for abort or okay</td>
</table>

## License

YASAB is released under the [LGPLv3](http://www.gnu.org/licenses/lgpl-3.0.html).
> &copy; 2012 Thomas Buck

See the accompanying COPYING file.
