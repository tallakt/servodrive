EXTRA_CFLAGS+=	-Wno-declaration-after-statement

MODULES	= servodrive

default:	all

obj-m += $(MODULES:%=%.o)

BUILD	= $(MODULES:%=%.ko)

all::	$(BUILD)

clean::
	rm -f $(BUILD) *.o *.ko *.mod.c *.mod.o *~ .*.cmd Module.symvers
	rm -rf .tmp_versions

$(MODULES:%=%.ko):	*.c
	$(MAKE) -C $(KERNEL_SOURCE) ARCH=$(ARCH) \
		SUBDIRS=$(CURDIR) CC=${CC}  modules

