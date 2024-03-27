KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

obj-m   := pciev.o
pciev-objs	:= main.o pci.o

default:
		$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

.PHONY: format
format:
	clang-format -i *.[ch]

.PHONY: clean
clean:
	   $(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	   rm -f cscope.out tags nvmev.S