#CFLAGS = -DTEST
#OBJS = clock.o console.o

BINARY = spi-mems

OOCD_INTERFACE = stlink-v2-1

LDSCRIPT = ../../../../../libopencm3-plus/lib/libopencm3_plus_stm32f429idiscovery.ld

LDLIBS		+= -lopencm3_plus_stm32f429idiscovery -lm

include ../../Makefile.include
