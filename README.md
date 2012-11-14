# YASAB
## Quickstart

To get the AVR bootloader, run make in the bootloader directory. This will produce yasab.hex to flash on your AVR. Change the target and other options in the makefile.

To get the upload utility, run the following commands in the uploader directory:

<pre>./configure
make install</pre>

To install it into /usr/local/bin/

You need a Toolchain for your Hostmachine and for AVRs.
Change configuration options for the AVR firmware in the makefile in "bootloader".

## Yet Another Simple AVR Bootloader

This AVR Bootloader just gets data from the included uploader. This will be flashed and started.
Currently supported AVR MCUs:
> ATmega32

It is using my [avrSerial Library](https://github.com/xythobuz/avrSerial) for UART communications.

Running "make" will produce yasab.hex and sample.hex. The first is the real bootloader. The second is a small program to test the bootloader.

Configuration Options like Baudrate, Bootdelay or Clock Frequency can be changed in the makefile.

Currently, 1868 bytes in Flash are needed. Therefore, set the bootloader section size to 1024 Words.

## Uploader

This is a small utility to upload hex files to YASAB.
The optional parameter, q, can be a character that is sent to reset the target.
> Usage: uploader /dev/port /path/to.hex [q]

## Protocol

38400, 8N1, XON-XOFF will be used for data transmission.

<table>
<tr><th>Computer</th><th>AVR</th></tr>
<tr><td>Repeat any character</td><td></td></tr>
<tr><td></td><td>Respond with 'o'</td></tr>
<tr><td>Send 'c'</td><td></td></tr>
<tr><td></td><td>Respond with 'a'</td></tr>
<tr><td>Send 32bit target address (MSB first)</td><td></td></tr>
<tr><td></td><td>Respond with 'o' or 'e'</td></tr>
<tr><td>Send 32bit data length (MSB first)</td><td></td></tr>
<tr><td></td><td>Respond with 'o' or 'e'</td></tr>
<tr><td>Send data...</td><td></td></tr>
<tr><td></td><td>In between 'e' or 'o' for abort or okay</td>
</table>

## License

YASAB is released under a BSD 2-Clause License. See the accompanying COPYING file.
