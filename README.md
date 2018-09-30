# OPNMIDI-Player-Java
Implementation of OPNMIDI based MIDI-player for Android

It's a MIDI-player based on emulator of a Frequency Modulation chip Yamaha OPN2 (YM2612). This small MIDI-player made with using of [libOPNMIDI](https://github.com/Wohlstand/libOPNMIDI/) library.

# Key features

* OPN2 emulation
* Customizable FM patches (/sdcard/gm.wopn file only)
* Stereo sound
* Number of simulated soundcards can be specified as 1-100 (maximum channels 600!)
* Pan (binary panning, i.e. left/right side on/off)
* Pitch-bender with adjustable range
* Vibrato that responds to RPN/NRPN parameters
* Sustain enable/disable
* MIDI and RMI files support
* loopStart / loopEnd tag support (Final Fantasy VII)
* Use automatic arpeggio with chords to relieve channel pressure
* Support for multiple concurrent MIDI synthesizers (per-track device/port select FF 09 message), can be used to overcome 16 channel limit

# Download latest binary

https://github.com/Wohlstand/OPNMIDI-Player-Java/releases

# System requirements

* Recommended to have powerful CPU
* 4 MB free space

# How to install

* Put APK to your Android device
* Check in settings that custom applications installations are allowed
* Open APK via any file manager and confirm installation
* Allow external storage reading (needed to open MIDI-files from phone memory and SD Card)

# How to use

* Use "Open" button to open file dialog and select any MIDI, RMI or KAR file
* Setup any preferences (volume model, number of emulated chips, etc.)
* Press "Play/Pause" to start playing or press again to pause or resume
* Press "Restart" to begin playing of music from begin (needed to apply changes of some settings)
* Use "Open" again to select any other music file
* You can switch another application or lock screen, music playing will work in background.

# Tips

* This application audio playback may lag on various devices, therefore you can reduce number of emulated chips

More detailed about playing MIDI with this application you also can find on [libOPNMIDI library repo](https://github.com/Wohlstand/libOPNMIDI/)

