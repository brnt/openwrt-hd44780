hd44780
=======

hd44780 kernel module for OpenWRT using GPIOs

This module controls LCD dot-matrix displays based on the popular [Hitachi
HD44780](http://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller) chipset. Data and commands are passed to the display through a misc device located at `/dev/hd44780`. To write text to the LCD, just write your text into the device file like this (ash):

    echo 'hello world' > /dev/hd44780

To send commands to the unit, the first byte in the string sent to the device file must be 0x01. This can easily be sent using escape characters (ash again):

    printf '\x01\x01' > /dev/hd44780

In this example, command mode is enabled with the first `\x01` and the screen is cleared with the second `\x01`. Similar results can be achieved using escape characters when writing to the file from most programming languages.

Each discrete string written to the file is evaluated for the presence of the command character. So this sequence

    printf '\x01\x01' > /dev/hd44780
    echo 'hello world' > /dev/hd44780

would clear the screen and then write `hello world` to it. The second statement is sent to the device file as a separate string and evaluated independently. Here's a similar routine in [NodeJS](http://nodejs.org/):

    var fs = require('fs');
    var data_file = '/dev/hd44780';
    fs.writeFileSync(data_file, "\1\1");
    fs.writeFile(data_file, 'hello world\n' + Math.random()));

Newline characters are automatically converted to the LCD's next line command, so the output here would be:

    hello world
    0.13524234984187

Wiring Instructions
-------------------
(TODO)

Building the Module
-------------------
(TODO)


### Change Log ###
* **v 1.1** Added ability to send command characters to device file
* **v 1.0** Original code by bufferos
