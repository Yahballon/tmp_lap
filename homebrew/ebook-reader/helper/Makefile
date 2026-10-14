ifndef PS5_PAYLOAD_SDK
$(error PS5_PAYLOAD_SDK not configured)
endif
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
CFLAGS += -Wall -Wextra -O2
all: bookscan.elf
bookscan.elf: bookscan.c
	$(CC) $(CFLAGS) -o $@ $<
