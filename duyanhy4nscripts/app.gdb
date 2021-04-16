# openocd
#target extended-remote localhost:3333

# jlink
target extended-remote localhost:2338

monitor reset
monitor halt
load BUILD/DISCO_L475VG_IOT01A/GCC_ARM-DEBUG/firmware-st-b-l475e-iot01a.elf
symbol-file BUILD/DISCO_L475VG_IOT01A/GCC_ARM-DEBUG/firmware-st-b-l475e-iot01a.elf

break main

# custom
tui en
c
