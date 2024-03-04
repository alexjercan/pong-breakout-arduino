CC=avr-gcc
BUILD_DIR=dist
CFLAGS=-mmcu=atmega328p -Wall -Wextra -Os -DF_CPU=16000000UL -g

.PHONY: clean build copy

build: $(BUILD_DIR)/main.hex

copy: build
	avrdude -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:$(BUILD_DIR)/main.hex

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/main.o: main.c lc.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.elf: $(BUILD_DIR)/main.o
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/main.elf

$(BUILD_DIR)/main.hex: $(BUILD_DIR)/main.elf
	avr-objcopy -j .text -j .data -O ihex $(BUILD_DIR)/main.elf $(BUILD_DIR)/main.hex

