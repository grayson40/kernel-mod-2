obj-m += lkm_pa2_in.o lkm_pa2_out.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)


all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
