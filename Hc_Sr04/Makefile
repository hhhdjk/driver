KERNELDIR :=/home/china/NanoPi2/linux
CURRENT_PATH := $(shell pwd)

obj-m := Hc_Sr04.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
