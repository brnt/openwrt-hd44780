hd44780
=======

hd44780 kernel module for OpenWRT using GPIOs

This module controls LCD dot-matrix displays based on the popular [Hitachi
HD44780](http://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller) chipset. Data and commands are passed to the display through a misc device located at `/dev/hd44780`. To write text to the LCD, just write your text into the device file like this (ash):

    echo -n 'hello world' > /dev/hd44780

To send commands to the unit, the first byte in the string sent to the device file must be 0x01. This can easily be sent using escape characters (ash again):

    printf '\x01\x01' > /dev/hd44780

In this example, command mode is enabled with the first `\x01` and the screen is cleared with the second `\x01`. Similar results can be achieved using escape characters when writing to the file from most programming languages. Several commands can be chained together. This command:

	printf '\x01\x01\xc6\x0f' > /dev/hd44780

will clear the display (`0x01`), move the cursor to the 6th position of the second row (`0xc6`) and make the cursor blink (`0x04`).

Each discrete string written to the file is evaluated for the presence of the command character. So this sequence

    printf '\x01\x01' > /dev/hd44780
    echo -n 'hello world' > /dev/hd44780

would clear the screen and then write `hello world` to it. The second statement is sent to the device file as a separate string and evaluated independently. Here's a similar routine in [NodeJS](http://nodejs.org/):

    var fs = require('fs');
    var data_file = '/dev/hd44780';
    fs.writeFileSync(data_file, "\1\1");
    fs.writeFile(data_file, 'hello world\n' + Math.random()));

Newline characters are automatically converted to the LCD's next line command, so the output here would be:

    hello world
    0.13524234984187

And here's a similar thing, in Lua this time:

	local lcd, err = io.open('/dev/hd44780', 'w')
	if not lcd then return print(err) end
	lcd:write(string.format('%c%c%c', 0x01, 0x01, 0xf))
	lcd:flush()
	lcd:write('hello world')
	lcd:close()

The command characters must be encoded with `string.format()`. Note that in Lua you must call `flush()` to end the command input and start another set of input (data this time).


Wiring Instructions
-------------------
(TODO)

Building the Module
-------------------
Copy the hd44780 folder into the `package` folder in your OpenWRT build root. From the build root, run

	./scripts/feeds update -i

This updates the package index, making the module you just added visible in menuconfig. Next run

	make menuconfig

and select the option for `kmod-hd44780` (Kernel Modules -> Other modules). Now the module will be built along with OpenWRT. To build just the module itself, run

	make package/hd44780/compile


### Change Log ###
* **v 1.1** Added ability to send command characters to device file
* **v 1.0** Original code by bufferos
