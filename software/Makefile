BUILD_DIR=../build/lattice_crosslink_nx_vip

include $(BUILD_DIR)/software/include/generated/variables.mak
include $(SOC_DIRECTORY)/software/common.mak

OBJECTS=isr.o main.o camera.o lcd.o

all: software.bin

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	chmod -x $@

software.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) \
		-T linker.ld \
		-N -o $@ \
		$(BUILD_DIR)/software/bios/crt0.o \
		$(OBJECTS) \
		-L$(BUILD_DIR)/software/libbase \
		-L$(BUILD_DIR)/software/libc \
		-L$(BUILD_DIR)/software/libcompiler_rt \
		-lc -lbase -lcompiler_rt
	chmod -x $@

main.o: main.c
	$(compile)

%.o: %.c
	$(compile)

%.o: %.S
	$(assemble)

clean:
	$(RM) $(OBJECTS) $(OBJECTS:.o=.d) software.elf software.bin .*~ *~

load: software.bin
	$(SOC_DIRECTORY)/../tools/litex_term.py  --serial-boot --kernel $< --kernel-adr 0x50000000  /dev/ttyUSB1

.PHONY: all main.o clean load
