# fsl

A data description language for file systems and structured binary data.


## Rationale

File systems are ubiquitous yet difficult to write.
On-disk structures are dynamic, bit-oriented, and must be serialized by hand-written functions.
Performance relies on efficient I/O ordering and using the most up-to-date API.
Metadata is cached by hand into space-efficient or alignment-friendly memory formats.
Errors accessing structures (whether from a bad I/O operation or plain bad data) are often mishandled.
Write ordering is controlled through either meticulous lock management or a master serialization queue.
These are hard problems that are revisited with every new file system.
The ultimate goal, however, is to map an optimized disk layout into a general set of operations.
Yet despite the challenge of implementing just a single file system correctly,
there is a proliferation of independent, isolated file systems used in practice.


FSL is a compiled declarative data definition language which is capable of succinctly expressing dynamic structures and organization rules.
File systems written as FSL source files are translated into runtime information which serves as a
generalized interface library.
To validate the feasibility of the language, FSL supports
iso9660 (a read-only CD-ROM file system),
VFAT (a legacy PC file system with files stored as linked lists),
ext2 (a traditional Unix-style file system with fixed-level trees), and
ReiserFS (a B+tree structured file system) in FSL,
which represent a broad spectrum of file system design choices.
Porting a file system to FSL is primarily an exercise in decoding disk layouts from debug utilities.


## Building

To build, run 'make'. To run tests, run 'make tests'.

## TODO

* adding new file systems
* using tools
* writing tools
* describe kernel mode
