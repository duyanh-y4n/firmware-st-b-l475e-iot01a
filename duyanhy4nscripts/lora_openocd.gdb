# openocd
#target extended-remote localhost:3333

# jlink
target extended-remote localhost:2338 

monitor reset 
monitor halt
#load build/src/apps/LoRaMac/LoRaMac-classA
#symbol-file build/src/apps/LoRaMac/LoRaMac-classA
load build/src/apps/porting_test/porting_test
symbol-file build/src/apps/porting_test/porting_test
#load build/src/apps/LoRaMac/LoRaMac-periodic-uplink-lpp
#symbol-file build/src/apps/LoRaMac/LoRaMac-periodic-uplink-lpp
#load build/src/apps/tx-cw/tx-cw
#symbol-file build/src/apps/tx-cw/tx-cw
#load build/src/apps/ping-pong/ping-pong
#symbol-file build/src/apps/ping-pong/ping-pong

break main

# custom
tui en
c
