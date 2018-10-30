## [M1: point 1]
#  The title of module is stated here
#  ...
MODULE	 = proj2

## [M2: point 1]
#  we are adding module object. MODULE in this case is the macro previously defined as "ex4"
#  ...
obj-m += $(MODULE).o

## [M3: point 1]
#  we are defining the macro called KERNELDIR here which will be what is provided in the following
#  ?= statement. ?= in this case is basically indicating to set KDIR variable only if it is not set
#  or doesn't have the value
#  ...
KERNELDIR ?= /lib/modules/$(shell uname -r)/build

## [M4: point 1]
#  defining the macro called PWD which will simply mimic the "pwd" call in our terminal shell
#  to find out what is our current directory address.
#  ...
PWD := $(shell pwd)

## [M5: point 1]
#  all target is defined so that we can call make all which will find everything that is defined
#  via MODULE macro (in this case ex4)
#  ...
all: $(MODULE)


## [M6: point 1]
#  rule is defined for any c files
#  $$< = name of the first prerequiste
#  $$@ = file name of the target of the rule.
#  ...
%.o: %.c
	@echo "  CC      $<"
	@$(CC) -c $< -o $@

## [M7: point 1]
#  creating rule for when make is called.
#  ...
$(MODULE):
	@echo "Hi" $(KERNELDIR)
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

## [M8: point 1]
#  clean target is defined to clean up everything when make clean is claled.
#  ...
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
