/*
 * Copyright (C) 2005-2006 Micronas USA Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/i2c.h>
#include <linux/videodev.h>
#include <linux/video_decoder.h>

#include "wis-i2c.h"

struct wis_tw9906 {
	int norm;
	int brightness;
	int contrast;
	int hue;
};

static u8 initial_registers[]=
{
	0x02, 0x40, /* input 0, composite */
	0x03, 0xa2, /* correct digital format */
	0x05, 0x81, /* or 0x01 for PAL */
	0x07, 0x02, /* window */
	0x08, 0x14, /* window */
	0x09, 0xf0, /* window */
	0x0a, 0x10, /* window */
	0x0b, 0xd0, /* window */
	0x0d, 0x00, /* scaling */
	0x0e, 0x11, /* scaling */
	0x0f, 0x00, /* scaling */
	0x10, 0x00, /* brightness */
	0x11, 0x60, /* contrast */
	0x12, 0x11, /* sharpness */
	0x13, 0x7e, /* U gain */
	0x14, 0x7e, /* V gain */
	0x15, 0x00, /* hue */
	0x19, 0x57, /* vbi */
	0x1a, 0x0f,
	0x1b, 0x40,
	0x29, 0x03,
	0x55, 0x00,
	0x6b, 0x26,
	0x6c, 0x36,
	0x6d, 0xf0,
	0x6e, 0x41,
	0x6f, 0x13,
	0xad, 0x70,
	0x00, 0x00, /* Terminator (reg 0x00 is read-only) */
};

static int write_reg(struct i2c_client *client, u8 reg, u8 value)
{
	return i2c_smbus_write_byte_data(client, reg, value);
}

static int write_regs(struct i2c_client *client, u8 *regs)
{
	int i;

	for (i = 0; regs[i] != 0x00; i += 2)
		if (i2c_smbus_write_byte_data(client, regs[i], regs[i + 1]) < 0)
			return -1;
	return 0;
}

static int wis_tw9906_command(struct i2c_client *client,
				unsigned int cmd, void *arg)
{
	struct wis_tw9906 *dec = i2c_get_clientdata(client);

	switch (cmd) {
	case DECODER_SET_INPUT:
	{
		int *input = arg;

		i2c_smbus_write_byte_data(client, 0x02, 0x40 | (*input << 1));
		break;
	}
	case DECODER_SET_NORM:
	{
		int *input = arg;
		u8 regs[] = {
			0x05, *input == VIDEO_MODE_NTSC ? 0x81 : 0x01,
			0x07, *input == VIDEO_MODE_NTSC ? 0x02 : 0x12,
			0x08, *input == VIDEO_MODE_NTSC ? 0x14 : 0x18,
			0x09, *input == VIDEO_MODE_NTSC ? 0xf0 : 0x20,
			0,	0,
		};
		write_regs(client, regs);
		dec->norm = *input;
		break;
	}
	case VIDIOC_QUERYCTRL:
	{
		struct v4l2_queryctrl *ctrl = arg;

		switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
			ctrl->type = V4L2_CTRL_TYPE_INTEGER;
			strncpy(ctrl->name, "Brightness", sizeof(ctrl->name));
			ctrl->minimum = -128;
			ctrl->maximum = 127;
			ctrl->step = 1;
			ctrl->default_value = 0x00;
			ctrl->flags = 0;
			break;
		case V4L2_CID_CONTRAST:
			ctrl->type = V4L2_CTRL_TYPE_INTEGER;
			strncpy(ctrl->name, "Contrast", sizeof(ctrl->name));
			ctrl->minimum = 0;
			ctrl->maximum = 255;
			ctrl->step = 1;
			ctrl->default_value = 0x60;
			ctrl->flags = 0;
			break;
		case V4L2_CID_HUE:
			ctrl->type = V4L2_CTRL_TYPE_INTEGER;
			strncpy(ctrl->name, "Hue", sizeof(ctrl->name));
			ctrl->minimum = -128;
			ctrl->maximum = 127;
			ctrl->step = 1;
			ctrl->default_value = 0;
			ctrl->flags = 0;
			break;
		}
		break;
	}
	case VIDIOC_S_CTRL:
	{
		struct v4l2_control *ctrl = arg;

		switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
			if (ctrl->value > 127)
				dec->brightness = 127;
			else if (ctrl->value < -128)
				dec->brightness = -128;
			else
				dec->brightness = ctrl->value;
			write_reg(client, 0x10, dec->brightness);
			break;
		case V4L2_CID_CONTRAST:
			if (ctrl->value > 255)
				dec->contrast = 255;
			else if (ctrl->value < 0)
				dec->contrast = 0;
			else
				dec->contrast = ctrl->value;
			write_reg(client, 0x11, dec->contrast);
			break;
		case V4L2_CID_HUE:
			if (ctrl->value > 127)
				dec->hue = 127;
			else if (ctrl->value < -128)
				dec->hue = -128;
			else
				dec->hue = ctrl->value;
			write_reg(client, 0x15, dec->hue);
			break;
		}
		break;
	}
	case VIDIOC_G_CTRL:
	{
		struct v4l2_control *ctrl = arg;

		switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
			ctrl->value = dec->brightness;
			break;
		case V4L2_CID_CONTRAST:
			ctrl->value = dec->contrast;
			break;
		case V4L2_CID_HUE:
			ctrl->value = dec->hue;
			break;
		}
		break;
	}
	default:
		break;
	}
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
static int wis_tw9906_i2c_id = 0;
#endif
static struct i2c_driver wis_tw9906_driver;

static struct i2c_client wis_tw9906_client_templ = {
	.name		= "TW9906 (WIS)",
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
	.flags		= I2C_CLIENT_ALLOW_USE,
#endif
	.driver		= &wis_tw9906_driver,
};

static int wis_tw9906_detect(struct i2c_adapter *adapter, int addr, int kind)
{
	struct i2c_client *client;
	struct wis_tw9906 *dec;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return 0;

	client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (client == NULL)
		return -ENOMEM;
	memcpy(client, &wis_tw9906_client_templ,
			sizeof(wis_tw9906_client_templ));
	client->adapter = adapter;
	client->addr = addr;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
	client->id = wis_tw9906_i2c_id++;
#endif

	dec = kmalloc(sizeof(struct wis_tw9906), GFP_KERNEL);
	if (dec == NULL) {
		kfree(client);
		return -ENOMEM;
	}
	dec->norm = VIDEO_MODE_NTSC;
	dec->brightness = 0;
	dec->contrast = 0x60;
	dec->hue = 0;
	i2c_set_clientdata(client, dec);

	printk(KERN_DEBUG
		"wis-tw9906: initializing TW9906 at address %d on %s\n",
		addr, adapter->name);

	if (write_regs(client, initial_registers) < 0)
	{
		printk(KERN_ERR "wis-tw9906: error initializing TW9906\n");
		kfree(client);
		kfree(dec);
		return 0;
	}

	i2c_attach_client(client);
	return 0;
}

static int wis_tw9906_detach(struct i2c_client *client)
{
	struct wis_tw9906 *dec = i2c_get_clientdata(client);
	int r;

	r = i2c_detach_client(client);
	if (r < 0)
		return r;

	kfree(client);
	kfree(dec);
	return 0;
}

static struct i2c_driver wis_tw9906_driver = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
	.owner		= THIS_MODULE,
	.name		= "WIS TW9906 I2C driver",
#else
	.driver = {
		.name	= "WIS TW9906 I2C driver",
	},
#endif
	.id		= I2C_DRIVERID_WIS_TW9906,
	.detach_client	= wis_tw9906_detach,
	.command	= wis_tw9906_command,
};

static int __init wis_tw9906_init(void)
{
	int r;

	r = i2c_add_driver(&wis_tw9906_driver);
	if (r < 0)
		return r;
	return wis_i2c_add_driver(wis_tw9906_driver.id, wis_tw9906_detect);
}

static void __exit wis_tw9906_cleanup(void)
{
	wis_i2c_del_driver(wis_tw9906_detect);
	i2c_del_driver(&wis_tw9906_driver);
}

module_init(wis_tw9906_init);
module_exit(wis_tw9906_cleanup);

MODULE_LICENSE("GPL v2");