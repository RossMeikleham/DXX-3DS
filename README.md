# DXX Switch

[![Build Status](https://travis-ci.org/aagallag/DXX-Switch.svg?branch=master)](https://travis-ci.org/aagallag/DXX-Switch)

A Nintendo Switch port of Descent I & Descent II.  This is a fork of [DXX-Retro](https://github.com/CDarrow/DXX-Retro), which is a fork of [DXX-Rebirth](https://github.com/dxx-rebirth/dxx-rebirth), which is a fork of the original engine source code created by Parallax Software Corporation.

## Install instructions
### D1X Switch
- Create a folder on your sdcard, such as `/switch/d1x-switch/`.
- Copy `d1x-switch.nro` and `descent.cfg` to this directory.  [Download the latest release here.](https://github.com/aagallag/DXX-Switch/releases)
- Copy `descent.hog` and `descent.pig` to the same directory.  These files are included with a purchase of the game.  Tested with [Descent I from Steam](https://store.steampowered.com/app/273570/Descent/).

### D2X Switch
- Create a folder on your sdcard, such as `/switch/d2x-switch/`.
- Copy `d2x-switch.nro` and `descent.cfg` to this directory.  [Download the latest release here.](https://github.com/aagallag/DXX-Switch/releases)
- Copy `descent2.ham`, `descent2.hog`, `descent2.s22`, and all `*.pig` files to the same directory.  These files are included with a purchase of the game.  Tested with [Descent II from Steam](https://store.steampowered.com/app/273580/Descent_2/).

## Known issues and missing features
- Multiple save files don't work (currently hardcoded to `player.plr`)
- Doesn't support the joysticks
- No music
- Only tested with the Descent files from Steam; GOG and CD-ROM releases have not been tested
- Doesn't utilize 1080p resolution when docked
- Need an icon
- Resize the text for larger resolutions (Bug only on D1X Switch)
- Enable the original HUD
- Network/Multiplayer is not enabled
- Level Selection menu doesn't work (no way to input a different level)

## Compile instructions

### Dependencies
- [devkitPro/libnx](https://devkitpro.org/wiki/devkitPro_pacman)

### Linux Compile Instructions

Start by compiling and installing SDL-1.2 and SDL_mixer for libnx
```
git clone git@github.com:aagallag/SDL-SWITCH.git -b master-aagallag
cd SDL-SWITCH/
make SDL
cd SDL-1.2.15/
make install
cd ../
make SDL_mixer
cd SDL_mixer-1.2.12/
make install
```

Then compile DXX Switch.
```
git clone --recurse-submodules git@github.com:aagallag/DXX-Switch.git
cd DXX-Switch/
make
```

## License
See [COPYING.txt](COPYING.txt)
