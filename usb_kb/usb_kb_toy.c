#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/hid.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luis de Bethencourt");

/* Table of devices that work with this driver */
static struct usb_device_id id_table[] = {
	{ USB_INTERFACE_INFO(
	  USB_INTERFACE_CLASS_HID,
	  USB_INTERFACE_SUBCLASS_BOOT,
	  USB_INTERFACE_PROTOCOL_KEYBOARD) },
	{ }
};

/* Register device table to load driver automatically */
MODULE_DEVICE_TABLE(usb, id_table);

static int __init usb_kb_toy_init(void)
{
	pr_err("USB Keyboard module init\n");

	return 0;
}

static void __exit usb_kb_toy_exit(void)
{
	pr_err("USB keyboard module exit\n");
}

module_init(usb_kb_toy_init);
module_exit(usb_kb_toy_exit);
