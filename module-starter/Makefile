obj-m += my-module.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Detect if Kernel is compiled with LLVM. Then use LLVM here as well.
KERNEL_CLANG := $(shell grep -q CONFIG_CC_IS_CLANG=y $(KDIR)/.config && echo 1 || echo 0)

ifeq ($(KERNEL_CLANG), 1)
	LLVM=1
endif

all:
	bear -- $(MAKE) -C $(KDIR) M=$(PWD) LLVM=$(LLVM) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
