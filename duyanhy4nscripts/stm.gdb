# openocd
#target extended-remote localhost:3333

# jlink
target extended-remote localhost:2338 

monitor reset 
monitor halt
load BUILD/NUCLEO_F767ZI/GCC_ARM/mbed-os-example-blinky.elf
symbol-file BUILD/NUCLEO_F767ZI/GCC_ARM/mbed-os-example-blinky.elf

break main

# custom
tui en
c
