# add additional build flag
BUILD_FLAGS=build_flags.txt # example: -D MY_MACRO -D MY_VALUE=1
CFLAGS= $(shell cat $(BUILD_FLAGS))

################################################################################
# MAKE PROFILE
################################################################################
# recommended: run mbed cli with poetry shell!!

.PHONY: all
all: clean build debug debug
clean:
	rm -rf BUILD

build:
	mbed compile -t GCC_ARM -m DISCO_L475VG_IOT01A --profile=debug

fast_clean_build:
	# remove compiled model and app source
	rm -rf BUILD/DISCO_L475VG_IOT01A/GCC_ARM-DEBUG/source
	# recompile
	mbed compile -t GCC_ARM -m DISCO_L475VG_IOT01A --profile=debug --color $(CFLAGS)

debug_build:
	mbed compile -t GCC_ARM -m DISCO_L475VG_IOT01A --profile=debug --color $(CFLAGS)

debug:
	sh duyanhy4nscripts/debug

doxygen:
	doxygen Doxyfile

mbed_update_lib:
	mbed deploy
