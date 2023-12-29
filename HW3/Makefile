PWD := $(shell pwd) 
KVERSION := $(shell uname -r)
KERNEL_DIR = /lib/modules/$(shell uname -r)/build

MODULE_NAME = kfetch_mod_311553060
obj-m := $(MODULE_NAME).o

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

load:
	sudo insmod $(MODULE_NAME).ko

unload:
	sudo rmmod $(MODULE_NAME)

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean