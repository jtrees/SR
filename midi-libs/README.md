# midi-libs

Libraries that are used by plugins in *midi-plugins* subproject to play MIDI music.

## wildmidi

[WildMidi](https://sourceforge.net/projects/wildmidi/ "WildMidi Midi Library and Player") is a library that uses GUS sound patches to play MIDI music.

The directory *wildmidi-0.2.3.5svn* contains modified wildmidi library version 0.2.3.5svn.

When compiling, first run `./bootstrap`, then `./configure` and `make`.

## wildmidiA

WildMidiA is a library that's used in Albion to play background effects, that are played using MIDI (not General MIDI).

The directory *wildmidiA-0.2.3.5svn* contains the library, which is a modification of wildmidi library.

When compiling, first run `./bootstrap`, then `./configure` and `make`.
