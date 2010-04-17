obj-m := servoctrldrv.o

PWD := $(shell pwd)

all: servoctrldrv.c
	echo $(PWD)
	make CROSS_COMPILE=arm-angstrom-linux-gnueabi- -C /home/tallakt/OE/angstrom-dev/staging/beagleboard-angstrom-linux-gnueabi/kernel M=$(PWD) modules

clean:
	-rm -rfd .s*
	-rm -rfd .t*
	-rm *.o
	-rm *.ko
	-rm *.mod.c
	-rm modules.order
	-rm Module.symvers

copy:
	sudo cp mkservodev /media/$(DISK)/home/root
	sudo cp servoctrldrv.ko /media/$(DISK)/home/root
