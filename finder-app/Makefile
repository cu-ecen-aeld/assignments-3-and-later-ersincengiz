#CROSS_COMPILE =/home/ersin/Documents/arm-inst/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc

#CROSS_COMPILE ?= aarch64-none-linux-gnu-
#CC = $(CROSS_COMPILE)gcc
CROSS_COMPILE ?= gcc

ifeq ($(CROSS_COMPILE),gcc)
    CC := gcc
else
    CC := $(CROSS_COMPILE)gcc
endif

writer: writer.c
	$(CC) writer.c -o writer
	
clean:
	rm -f writer .o

export PATH := /home/ersin/Documents/arm-inst/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin:$(PATH)
