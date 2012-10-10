# YASAB
## Yet Another Simple AVR Bootloader

This AVR Bootloader just reads a HEX File from the serial Port and answers with human-readable output. It uses XON-XOFF Flow Control with 38400bps, 8N1.
Currently supported AVR MCUs:
> ATmega32

It is using my [AvrSerialLibrary](https://github.com/xythobuz/Snippets/tree/master/AvrSerialLibrary) for UART communications.

Running "make" will produce yasab.hex and sample.hex. The first is the real bootloader. The second is a small program to test the bootloader.

At the moment, AVRs with Flash > 64KB can not be programmed, as the parser can only read type 1 hex file records.

Configuration Options like Baudrate, Bootdelay or Clock Frequency can be changed in the makefile.

## License

YASAB is released under the [LGPLv3](http://www.gnu.org/licenses/lgpl-3.0.html).
> &copy; 2012 Thomas Buck

See the accompanying COPYING file.
