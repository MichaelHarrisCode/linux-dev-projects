obj-m += my_rust.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

KERNEL_CLANG := $(shell grep -q CONFIG_CC_IS_CLANG=y $(KDIR)/.config && echo 1 || echo 0)

ifeq ($(KERNEL_CLANG),1)
	LLVM=1
#else
#	$(warning Kernel not compiled with LLVM. Rust module may not compile correctly)
endif

all:
	make -C $(KDIR) M=$(PWD) LLVM=$(LLVM) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
