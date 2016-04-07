KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += usb_kb_toy.o

all:
	make -C $(KERNELDIR) M=$(PWD) modules
	modinfo ./usb_kb_toy.ko

install:
	cp usb_kb_toy.ko /lib/modules/$(shell uname -r)/
	depmod

uninstall:
	rm /lib/modules/$(shell uname -r)/usb_kb_toy.ko
	depmod

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
