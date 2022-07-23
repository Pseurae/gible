<div style="display: block; position: relative;">
<h1 style="width: 100%;">Gible</h1>
<p style="position: absolute; margin: auto; top: 0; bottom: 0; right: 0;">By <b>@Pseurae</b></p>
</div>

A ROM patcher made using C.

Gible supports patching of IPS, IPS32, UPS and BPS files.  Patch creation and other patch formats are planned to be added in the near future, along with Windows support.

## Building

### Windows
Doesn't work on Windows yet. I still don't have a Windows device on hand to code in filemapping using `MapFileToView`.

### Linux & Mac
Literally just run `make` and it should compile into an executable file.

## External Resources

#### CRC32 Implementation
Credits go to Stephan Brumme.  
http://create.stephan-brumme.com/disclaimer.html

#### [ARGC](https://gist.github.com/Pseurae/b412b4b7cdceff1ce2b8c0561f81f339)
Argument parser made for Gible.