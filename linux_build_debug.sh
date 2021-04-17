#!/usr/bin/env bash

poetry run bear --append -- make debug_build -j8 CFLAGS=""
