.PHONY: all clean build debug call_graph
clean:
	rm -rf BUILD

build:
	mbed compile -t GCC_ARM -m DISCO_L475VG_IOT01A --profile=debug

debug_build:
	mbed compile -t GCC_ARM -m DISCO_L475VG_IOT01A --profile=debug

call_graph:
	sh gen_call_graph.sh source/*.cpp
	sh gen_call_graph.sh source/edge-impulse-sdk/classifier/*.cpp
	sh gen_call_graph.sh source/sensors/*.cpp
	sh gen_call_graph.sh source/tflite-model/*.cpp
	sh gen_call_graph.sh source/edge-impulse-sdk/tensorflow/lite/c/*.c
	sh gen_call_graph.sh source/edge-impulse-sdk/tensorflow/lite/core/api/*.cc
