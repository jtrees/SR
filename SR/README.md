# SR

The static recompiler itself.

This utility takes an original executable and information about the original executable as an input and produces x86 or arm assembler version of the executable.

The generated assembler version requires additional files to compile and run. The non-specific files are in the *x86-support* and *arm-support* subdirectories.

The utility uses [Judy](http://judy.sourceforge.net/ "Judy Arrays Web Page") library and modified [udis86](http://udis86.sourceforge.net/ "Udis86 Disassembler Library for x86 / x86-64") library (version 1.6) -- the modified source is in the *udis86-1.6* subdirectory.

The utility can be built using [scons](http://scons.org/ "SCons: A software construction tool") -- the result is an executable file *SR.exe*.

By default a version that generates x86 assembler code is compiled. To compile a version that generates arm assembler code, change the definition of OUTPUT_TYPE from OUT_X86 to OUT_ARM_LINUX in file *SR_defs.h*.

