ARM_PREFIX = arm-none-eabi-
OPENCM3_DIR = ./libopencm3

CC = gcc
LD = gcc
RM = rm
OBJCOPY = objcopy

ARM_ARCH_FLAGS = -mthumb -mcpu=cortex-m0plus -msoft-float

ARM_CFLAGS = -I$(OPENCM3_DIR)/include -DSTM32L0 $(ARM_ARCH_FLAGS) -g
#ARM_CFLAGS += -fno-common -ffunction-sections -fdata-sections
ARM_LDLIBS = -lopencm3_stm32l0
ARM_LDSCRIPT = $(OPENCM3_DIR)/lib/stm32/l0/stm32l0xx4.ld
ARM_LDFLAGS = -L$(OPENCM3_DIR)/lib --static -nostartfiles -T$(ARM_LDSCRIPT)
ARM_LDFLAGS += $(ARM_ARCH_FLAGS)

host:
	$(CC) izhi.c host.c -o host.o

stm:
	$(ARM_PREFIX)$(CC) $(ARM_CFLAGS) -c izhi.c -o izhi.o
	$(ARM_PREFIX)$(CC) $(ARM_CFLAGS) -c stm.c -o stm.o
	$(ARM_PREFIX)$(LD) $(ARM_LDFLAGS) izhi.o stm.o $(ARM_LDLIBS) -o stm.elf
	$(ARM_PREFIX)$(OBJCOPY) -Obinary stm.elf stm.bin
	$(ARM_PREFIX)$(OBJCOPY) -Oihex stm.elf stm.hex

clean:
	$(RM) -f *.d *.o *.elf *.map *.bin *.hex
