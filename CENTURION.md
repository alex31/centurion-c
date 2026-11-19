# Centurion CPU6 instruction emulator

The `upstream` branch imports the source tree at
[EtchedPixels/Centurion commit 4e1a61c](https://github.com/EtchedPixels/Centurion/tree/4e1a61c56a48f36f231e93837f4896d621290685).

The `centurion` branch carries the instruction and disassembly fixes used
in the SDCC development tests. It corrects word increment flags, handles the
CPU6 multiply and divide operand encodings, and implements the multiword
division forms exercised by the tests.

The distribution uses `-R` with raw CPU6 test programs to start directly
at the program entry point without boot ROMs or wall-clock pacing.
Peripheral scheduling still follows emulated CPU time.
The emulator builds on demand for the compiler regression suites.

[Upstream license](LICENCE)
