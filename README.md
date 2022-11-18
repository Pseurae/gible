# Gible
By **@Pseurae**

A ROM patcher made using C.

Gible supports patching of IPS, IPS32, UPS and BPS files.  Patch creation and other patch formats are planned to be added in the near future, along with Windows support.

## Building

### Windows
Install mingw-w64 and run `make windows` on WSL. Same can be done in Mac or Linux.

### Linux & Mac
Just run `make` and gible should be compiled.

> You may need to run `make clean` if switching compilation between Windows and Unix.   

## External Resources

#### CRC32 Implementation
Credits go to Stephan Brumme.  
http://create.stephan-brumme.com/disclaimer.html

#### [ARGC](https://gist.github.com/Pseurae/b412b4b7cdceff1ce2b8c0561f81f339)
Argument parser made for Gible.
