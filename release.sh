#!/bin/sh
make
mv mra release/linux/
make -f makefile.windows
mv mra.exe release/windows
