/*
   hd44780 driver using gpio pins.

   Copyright (C) bifferos@yahoo.co.uk, 2008
   */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/gpio.h>


// Minor device number (major device is 'misc').
#define HD44780_MINOR 153
#define HD44780_COMMAND_MINOR 154


// Pin assignments for the board.  Change the GPIO numbers to match your
// wiring.

//  Pin ID     Linux GPIO number      Edimax/Omnima pin annotation
#define HD_RS       0              //        D6
#define HD_RW       8              //        D7
#define HD_E        1              //        D8
#define HD_DB4     13              //        D9
#define HD_DB5     14              //        D10
#define HD_DB6     15              //        D11
#define HD_DB7     16              //        D12


// Structures to hold the pins we've successfully allocated.
typedef struct PinSet {
	int pin;      // Linux GPIO pin number
	char* name;   // Name of the pin, supplied to gpio_request()
	int result;   // set to zero on successfully obtaining pin
} tPinSet;


static tPinSet pins[] = {
	{HD_E,   "HD44780_E",   -1},
	{HD_RW,  "HD44780_RW",  -1},
	{HD_RS,  "HD44780_RS",  -1},
	{HD_DB4, "HD44780_DB4", -1},
	{HD_DB5, "HD44780_DB5", -1},
	{HD_DB6, "HD44780_DB6", -1},
	{HD_DB7, "HD44780_DB7", -1},
};

#define PINS_NEEDED (sizeof(pins)/sizeof(tPinSet))


// Macros for setting control lines
#define HD_RS_LOW  gpio_set_value(HD_RS, 0);
#define HD_RS_HIGH gpio_set_value(HD_RS, 1);
#define HD_E_LOW   gpio_set_value(HD_E, 0);
#define HD_E_HIGH  gpio_set_value(HD_E, 1);


MODULE_AUTHOR("bifferos");
MODULE_LICENSE("GPL");


// ioctl - I/O control
static int hd44780_ioctl(struct inode *inode, struct file *file, 
		unsigned int cmd, unsigned long arg) 
{
	int data;

	// Read the request data
	if (copy_from_user(&data, (int *)arg, sizeof(data)))
	{
		printk(KERN_INFO "copy_from_user error on hd44780 ioctl.\n");
		return -EFAULT;
	}

	switch (cmd)
	{

		default:		
			printk(KERN_INFO "Invalid ioctl on hd44780\n");
			return -EINVAL;
	}

	// return the result
	if (copy_to_user((int *)arg, &data, 4))
	{
		printk(KERN_INFO "copy_to_user error on hd44780 ioctl\n");
		return -EFAULT;
	}

	return 0;
}

// Write a nibble to the display
static void WriteNibble(unsigned int val)
{
	HD_E_LOW;
	gpio_set_value(HD_DB4, (val&0x1));
	gpio_set_value(HD_DB5, (val&0x2)>>1);
	gpio_set_value(HD_DB6, (val&0x4)>>2);
	gpio_set_value(HD_DB7, (val&0x8)>>3);
	udelay(1);   // data setup time
	HD_E_HIGH;
	udelay(1);
	HD_E_LOW;
	udelay(1);    // data hold time
}

// Write char data to display
static void WriteData(char c)
{
	udelay(1);
	HD_RS_HIGH;
	udelay(1);
	WriteNibble((c>>4)&0xf);
	WriteNibble(c&0xf);
	udelay(50);
}

// Send command code to the display
static void WriteCommand(char c)
{
	udelay(1);
	HD_RS_LOW;
	udelay(1);
	WriteNibble((c>>4)&0xf);
	WriteNibble(c&0xf);
	udelay(50);
}

// Called when writing to the device file.
static ssize_t hd44780_write(struct file *file, const char *buf, size_t count, loff_t *ppos) 
{
	int err;
	char c;
	const char* ptr = buf;
	int i;
	for (i=0;i<count;i++)
	{
		err = copy_from_user(&c,ptr++,1);
		if (err != 0)
			return -EFAULT;
		// Write the byte to the display.
		if (c == '\n') {
			WriteCommand(0xC0);
		} else {
			WriteData(c);
		}
	}
	return count;
}

static struct file_operations hd44780_fops = {
	.owner = THIS_MODULE,
	//  .ioctl = hd44780_ioctl,  ioctl not supported for now.
	.write = hd44780_write,
};

static struct miscdevice hd44780_device = {
	HD44780_MINOR,
	"hd44780",
	&hd44780_fops,
};

// Called when writing to the command file
static ssize_t hd44780_write_command(struct file *file, const char *buf, size_t count, loff_t *ppos) 
{
	int err;
	char c;
	const char* ptr = buf;
	int i;
	for (i=0;i<count;i++)
	{
		err = copy_from_user(&c,ptr++,1);
		if (err != 0)
			return -EFAULT;
		// Write the byte to the display.
		WriteCommand(c);
	}
	return count;
}
static struct file_operations hd44780_command_fops = {
	.owner = THIS_MODULE,
	//  .ioctl = hd44780_ioctl,  ioctl not supported for now.
	.write = hd44780_write_command,
};

static struct miscdevice hd44780_command_device = {
	HD44780_COMMAND_MINOR,
	"hd44780-command",
	&hd44780_command_fops,
};


// Return any acquired pins.
static void FreePins(void)
{
	int i;
	for (i=0;i<PINS_NEEDED;i++)
	{
		if (pins[i].result == 0)
		{
			gpio_free(pins[i].pin);
			pins[i].result = -1;    // defensive programming - avoid multiple free.
		}
	}
}

static int __init hd44780_init(void)
{
	int i;
	int got_pins = 1;

	// Register misc device
	if (misc_register(&hd44780_device)) {
		printk(KERN_WARNING "Couldn't register device %d\n", HD44780_MINOR);
		return -EBUSY;
	}

	if (misc_register(&hd44780_command_device)) {
		printk(KERN_WARNING "Couldn't register command device %d\n", HD44780_COMMAND_MINOR);
		misc_deregister(&hd44780_device);
		return -EBUSY;
	}

	// Request pins
	for (i=0;i<PINS_NEEDED;i++)
	{
		pins[i].result = gpio_request(pins[i].pin,pins[i].name);
		if (pins[i].result != 0)
			got_pins = 0;
	}

	// On any failures, return any pins we managed to get and quit.
	if (!got_pins)
	{
		FreePins();
		return -EBUSY;
	}

	// Set port direction.  We assume we can do this if we got the pins.
	// Set initial values to low (0v).
	for (i=0;i<PINS_NEEDED;i++)
	{
		gpio_direction_output(pins[i].pin,0);
	}

	// Power on and setup the display
	WriteCommand(0x33);
	udelay(50);
	WriteCommand(0x32);
	udelay(50);
	WriteCommand(0x28);
	udelay(50);
	WriteCommand(0x0c);
	udelay(50);
	WriteCommand(0x01);
	udelay(50);
	WriteCommand(0x06);
	udelay(50);

	printk(KERN_INFO "hd44780 driver (v1.0) by bifferos, loaded.\n");
	return 0;
}

static void __exit hd44780_exit(void)
{
	FreePins();
	misc_deregister(&hd44780_device);
	misc_deregister(&hd44780_command_device);
	printk(KERN_INFO "hd44780 driver (v1.0) by bifferos, unloaded.\n");
}

module_init(hd44780_init);
module_exit(hd44780_exit);


