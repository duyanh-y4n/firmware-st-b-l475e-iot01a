# add additional build flag
BUILD_FLAGS=build_flags.txt # example: -D MY_MACRO -D MY_VALUE=1
CFLAGS= $(shell cat $(BUILD_FLAGS))

################################################################################
# MAKE PROFILE
################################################################################
# recommended: run mbed cli with poetry shell!!

.PHONY: all clean build debug call_graph debug
clean:
	rm -rf BUILD

build:
	mbed compile -t GCC_ARM -m DISCO_L475VG_IOT01A --profile=debug

debug_build:
	mbed compile -t GCC_ARM -m DISCO_L475VG_IOT01A --profile=debug $(CFLAGS)

debug:
	sh duyanhy4nscripts/debug

call_graph:
	sh gen_call_graph.sh source/*.cpp
	sh gen_call_graph.sh source/edge-impulse-sdk/classifier/*.cpp
	sh gen_call_graph.sh source/sensors/*.cpp
	sh gen_call_graph.sh source/tflite-model/*.cpp
	sh gen_call_graph.sh source/edge-impulse-sdk/tensorflow/lite/c/*.c
	sh gen_call_graph.sh source/edge-impulse-sdk/tensorflow/lite/core/api/*.cc
