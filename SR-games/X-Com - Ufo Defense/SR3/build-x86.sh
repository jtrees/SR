#! /bin/sh
cd "`echo $0 | sed 's/\/[^\/]*$//'`"
cp x86/*.sci ./
./SR.exe INTRO.EXE intro-asm.asm >a.a 2>b.a
rm *.sci
./compact_source.py
nasm -felf -dELF -O1 -w+orphan-labels -w-number-overflow -ix86/ intro-asm_linux.asm 2>a.a
./repair_short_jumps.py
rm *.a
