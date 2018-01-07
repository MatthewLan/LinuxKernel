
KVERS = $(shell uname -r)

# Kernel modules
# 1 - only one file
# obj-m += globalmem.o
obj-m += globalmem_block.o

# 2 - multiple files
# modulename-objs := globalmem.o
# obj-m += modulename.o

# Speciafy flags for the module compilation.
# EXTRA_CFLAGS=-g -O0

build: kernel_modules

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

clean:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
