#!/bin/sh

if [ $# -eq 0 ]
then
    gdb-multiarch ../build/kernel.elf -x init.gdb
else
    gdb-multiarch --batch ../build/kernel.elf -x $1
fi

stty sane
