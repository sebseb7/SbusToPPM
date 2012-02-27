PROJECT=sbus2ppm

MCU = atmega88
MCU_AVRDUDE = atmega88


#full swing, bod 2,7
FUSE_SETTINGS = -U lfuse:w:0xf7:m -U hfuse:w:0xdd:m -U efuse:w:0xf9:m


ifeq ($(OSTYPE),)
OSTYPE      = $(shell uname)
endif
ifneq ($(findstring Darwin,$(OSTYPE)),)
USB_DEVICE = /dev/cu.SLAB_USBtoUART
else
USB_DEVICE = /dev/ttyUSB1
endif


#########################################################################

SRC=$(wildcard *.c)
OBJECTS=$(SRC:.c=.o) 
DFILES=$(SRC:.c=.d) 
HEADERS=$(wildcard *.h)



#  Compiler Options
GCFLAGS = -mmcu=$(MCU) -I. -gstabs -DF_CPU=20000000 -O2 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wstrict-prototypes  -std=gnu99 -MD -MP
#LDFLAGS =  -Wl,-Map=pwbl.map,--cref    -lm -Wl,--section-start=.text=0x1800
LDFLAGS = -mmcu=$(MCU)  

#  Compiler Paths
GCC = avr-gcc
REMOVE = rm -f
SIZE = avr-size
OBJCOPY = avr-objcopy

#########################################################################

all: $(PROJECT).hex Makefile stats

$(PROJECT).hex: $(PROJECT).elf Makefile
	$(OBJCOPY) -R .eeprom -O ihex $(PROJECT).elf $(PROJECT).hex 

$(PROJECT).elf: $(OBJECTS) Makefile
	$(GCC) $(LDFLAGS) $(OBJECTS) -o $(PROJECT).elf

stats: $(PROJECT).elf Makefile
	$(SIZE)  -C --mcu=$(MCU)  $(PROJECT).elf

clean:
	$(REMOVE) $(OBJECTS)
	$(REMOVE) $(PROJECT).hex
	$(REMOVE) $(DFILES)
	$(REMOVE) $(PROJECT).elf

#########################################################################

%.o: %.c Makefile $(HEADERS)
	$(GCC) $(GCFLAGS) -o $@ -c $<

#########################################################################

flash: all
	avrdude -F -p $(MCU_AVRDUDE) -P $(USB_DEVICE) -c stk500v2    -U flash:w:$(PROJECT).hex

fuse:
	avrdude -F -p $(MCU_AVRDUDE) -P $(USB_DEVICE) -c stk500v2    $(FUSE_SETTINGS)

