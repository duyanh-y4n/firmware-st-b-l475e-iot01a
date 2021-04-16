.PHONY: all clean build debug
clean:
	rm -rf BUILD

build:
	mbed compile -t GCC_ARM -m DISCO_L475VG_IOT01A --profile=release

debug_build:
	mbed compile -t GCC_ARM -m DISCO_L475VG_IOT01A --profile=debug
