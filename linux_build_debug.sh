#!/usr/bin/env bash

if [[ $1 == "build" ]]; then
    poetry run bear --append -- make debug_build -j8
else [[ $1 == "clean" ]]
    rm -rf BUILD/DISCO_L475VG_IOT01A/GCC_ARM-DEBUG/source
fi

