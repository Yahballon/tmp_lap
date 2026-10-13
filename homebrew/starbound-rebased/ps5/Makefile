# Real Gold872 ECS / SDL2 PS5 payload prototype. Not a ShadowMount title yet.
# Requires a PS5-payload-sdk sysroot with PacBrew SDL2 + SDL2_image + SDL2_ttf + SDL2_mixer.
PS5_PAYLOAD_SDK ?= /opt/ps5-payload-sdk
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

UPSTREAM ?= ../../build/gold872-engine
HB := $(PS5_PAYLOAD_SDK)/target/user/homebrew
INC := -I$(UPSTREAM)/include -I$(HB)/include -I$(HB)/include/SDL2
SOURCES := $(shell find $(UPSTREAM)/src -name '*.cpp' -type f)
SDL_ARCHIVES := $(HB)/lib/libSDL2main.a $(HB)/lib/libSDL2.a \
  $(HB)/lib/libSDL2_image.a $(HB)/lib/libSDL2_mixer.a \
  $(HB)/lib/libSDL2_ttf.a $(HB)/lib/libfreetype.a
SYSLIBS := -lkernel_sys -lSceSystemService -lSceUserService \
  -lScePad -lSceVideoOut -lSceAudioOut -lSceKeyboard -lSceImeDialog

.PHONY: all check-deps clean
all: check-deps build/starbound-gold872.elf
check-deps:
	@test -s "$(UPSTREAM)/src/main.cpp" || (echo "Run tools/fetch-engine.sh first" >&2; exit 1)
	@for lib in $(SDL_ARCHIVES); do test -s "$$lib" || (echo "Missing PS5 SDL archive $$lib" >&2; exit 1); done
build:
	mkdir -p build
# The original Gold872 engine is compiled, NOT recreated.
# -include cstdint compensates for a missing header in the pinned upstream source.
build/starbound-gold872.elf: $(SOURCES) | build check-deps
	$(CXX) -std=c++17 -O1 -include cstdint $(INC) -o $@ $(SOURCES) \
	  $(SDL_ARCHIVES) -lbz2 -lpng -liconv -lm -lz $(SYSLIBS)
clean:
	rm -rf build
