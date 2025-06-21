#!/bin/sh
stty -echo raw
build/firmware.elf
res=$?
stty sane
exit $res
