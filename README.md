# DXX 3DS

[![Build Status](https://travis-ci.org/rossmeikleham/DXX-3DS.svg?branch=master)](https://travis-ci.org/rossmeikleham/DXX-3DS)

A Nintendo 3DS port of Descent I & Descent II.  This is a fork of [DXX-Switch](https://github.com/aagallag/DXX-Switch)/[DXX-Retro](https://github.com/CDarrow/DXX-Retro), which is a fork of [DXX-Rebirth](https://github.com/dxx-rebirth/dxx-rebirth), which is a fork of the original engine source code created by Parallax Software Corporation.

## Install instructions
### D1X Switch
- Create the folder /3ds/D1
- Copy `descent.hog`and `descent.pig` to the same directory. These files are included with a purchase of the game.  Tested with [Descent I from Steam](https://store.steampowered.com/app/273570/Descent/).

### D2X Switch
- Create the folder /3ds/D1
- Copy `descent2.ham, descent2.hog, descent2.s22`, and all `*.pig` files to the same directory.  These files are included with a purchase of the game.  Tested with [Descent II from Steam](https://store.steampowered.com/app/273580/Descent_2/).

## Music
- The 3DS is unable to play the hmp converted to midi files in the Descent games
- On first run of D1X/D2X, midi files from the respecitive game will be copied to/3ds/D1/midi/ and /3ds/D2/midi/
- These midi files can be converted to WAV though a program such as Timidity with Fluidr3 soundfonts [See here](https://wiki.archlinux.org/index.php/Timidity)
- Also GoG version apparently comes with MP3 versions of all the music files for each game, these should be able to be copied to /3ds/D1/mp3/ and /3ds/D2/mp3/ if the filenames match the midi names for each sequence/level
- Descent1/2 will attempt to first play music from /3ds/D1/wav and /3ds/D2/wav, then /3ds/D1/mp3 and /3ds/D2/mp3
- The names (not including the file extension) need to match the names of the midi songs but can be replaced with your own music if desired

## Known issues and missing features
- Unable to get a working .cia version (I think his is due to the large amount of stack space used by DXX Retro (around 4-8MB), default 3DS stack space is 32KB. Setting `__stacksize__` to 8MB works fine with homebrew browser, but .cia builds automatically crash) 
- Multiple save files don't work (currently hardcoded to `player.plr`)
- Only tested with the Descent files from Steam; GOG and CD-ROM releases have not been tested
- Enable the original HUD
- Network/Multiplayer is not enabled
- Level Selection menu doesn't work (no way to input a different level)

### D1X
- End of level "sequence" is skipped, was causing a segfault/crash

### D2X
- Movies disabled, these were also causing crashes 


# Building from Docker
```
git clone git@github.com:rossmeikleham/DXX-3DS.git
cd DXX-3DS
docker build -t descent .
docker run -v $(pwd):/mnt descent
```

This should produce `d1x-3ds.3dsx` `d1x-3ds.cia` d2x-3ds.3dsx`` d2x-3ds.cia` in the 
root of the repository

# Manually building

## Install libphysfs for ctru
Follow instructions from [physfs-3ds](https://github.com/rossmeikleham/physfs-3ds) repo page.

## Install bannertool + makerom (required for cia builds, ignore for 3dsx only)
- Build [bannertool](https://github.com/Steveice10/bannertool) and add it to the PATH
- Build [makerom](https://github.com/profi200/Project_CTR) and add it to the PATH

## Compile DXX 3DS (3dsx + cia)
```
git clone git@github.com:rossmeikleham/DXX-3DS.git
cd DXX-3DS/
./make_3ds.sh
```

## Compile DXX 3DS (3dsx only)
```
git clone git@github.com:rossmeikleham/DXX-3DS.git
cd DXX-3DS/
cd D1 && make && cp d1x-3ds.3dsx ../
cd ../D2 && make && cp d2x-3ds.3dsx ../
```

## License
See [COPYING.txt](COPYING.txt)
