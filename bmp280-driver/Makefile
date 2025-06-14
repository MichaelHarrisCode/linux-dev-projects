obj-m += bmp280.o

KDIR := ~/linux-dev/raspberrypi/linux
PWD := $(shell pwd)
ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-

all:
	bear -- $(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

send:
	scp bmp280.ko michael@raspberrypi.local:/home/michael/linux;

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
