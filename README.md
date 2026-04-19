<img src="https://github.com/VinxScorza/performer/actions/workflows/ci.yml/badge.svg?branch=master" alt="Build Status">

# Vinx PER|FORMER firmware v0.4.0

## <a href="CHANGELOG.md" target="_blank" rel="noopener noreferrer">Click for CHANGELOG</a> · <a href="https://vinxscorza.github.io/performer/features/" target="_blank" rel="noopener noreferrer">Click for FEATURES</a>

This is a personal fork of the <a href="https://github.com/mebitek/performer" target="_blank" rel="noopener noreferrer">Mebitek fork</a>, itself based on the original <a href="https://github.com/westlicht/performer" target="_blank" rel="noopener noreferrer">Westlicht PER|FORMER firmware</a>.

The Vinx Scorza line begins at `v0.3.2-vinx.1`. Everything before that point in this repository history and changelog is inherited from the Mebitek fork and kept here as upstream reference. Starting from the first standalone Vinx release, this fork uses standalone Vinx semantic versioning (`v0.x.y`) and no longer carries the inherited `v0.3.2-vinx.*` prefix. Historical entries remain unchanged as lineage reference.

If you are looking for a more conservative upstream baseline, you may prefer the original Westlicht or Mebitek lines. If you are interested in a more hands-on, performance-oriented evolution of PER|FORMER, you are in the right place. What I'm aiming for is a solid machine for live performance, but also a crazy one for experimenting.<br>
I am very grateful to Simon Kallweit for creating and developing the original Westlicht PER|FORMER. If you would like to support this fork and the upstream work behind it financially, you can donate here:<br>
<a href="https://vinxscorza.github.io/performer/donate/" target="_blank" rel="noopener noreferrer">Donate to Vinx Scorza</a> · <a href="https://mebitek.github.io/performer/donate/" target="_blank" rel="noopener noreferrer">Donate to Mebitek</a> · <a href="https://westlicht.github.io/performer/donate/" target="_blank" rel="noopener noreferrer">Donate to Simon Kallweit / Westlicht</a>.

Primary documentation for this fork: Vinx Scorza fork website, user manual, LP Cheatsheet, Web Simulator, features — <a href="https://vinxscorza.github.io/performer/" target="_blank" rel="noopener noreferrer">https://vinxscorza.github.io/performer/</a>

## Major Features

- `Core Sequencing`: Stabilized step engines across `Note`/`Logic`/`Curve`/`Stochastic`/`Arp`, including legacy crash paths, non-zero subrange shifting, refined step selection behavior (shift handling and selection-or-all fallback logic), Curve `Gate Offset`/`Gate Length`, restored Curve undo, and scale/midi-capture consistency improvements. `Voltage Mode` handling is now consistent across capture and playback paths: MIDI note capture follows the active scale octave span, and `Arp`/`Stochastic` bypass paths respect non-chromatic user scales (no forced semitone/`1V/oct` fallback).
- `Chaos generators`: `Chaos` evolved into `Vandalize Sequence` + `Wreck Pattern` with explicit `A/B` safety flow, selection-aware targeting, and machine-level defaults with separate masks. 
- `Acid generators`: `Acid` is now consolidated as `Layer` + `Phrase`, with immediate preview-on-entry, dedicated visuals, tightened parameter behavior, and aligned ordering/mapping across machine and Launchpad. 
- `Random generator`: `Random` now runs as a stable preview/apply workflow, controlled `Variation`, full `32-bit` seed handling, richer preview rendering, and footer actions that stay in-context. 
- `Launchpad`: Launchpad is treated as a first-class layer (`LP Style` / `LP Note Style`), defaults to `Blue + Circuit`, supports `Classic` and `Circuit`, includes many regression fixes, and adds split `Generators Mode` (`Note` full map with `Init Layer` + `Init Steps`; `Curve`/`Stochastic`/`Logic`/`Arp` subset with `Entropy`, `Init Layer`, and `Init Steps`), plus `1-level Undo/Redo`, lock policies, overlay stability, and a near-complete refactoring of the Launchpad controller codebase.
- `16-step Editing Mode`: Experimental external controller workflow for `Launch Control XL` and `BeatStep Pro`, using `16 knobs + 16 pads + 2 function buttons`. Target is the currently selected `Note` track: knobs edit `Note` only when machine layer is `Note`, while pads toggle `Gate` on the same track across layers. `BeatStep Pro` remains code-supported but still requires broader on-hardware validation; maintainer-side hardware tests so far are on `Launch Control XL` only.
- `Clock & Sync`: External clock behavior was hardened for real hardware (`Reset Gate`, `Reset Pulse`, rising-edge-only pulse handling, `Reset CV` default `Off`) while keeping backward-compatible timing behavior. 
- `Generator UX`: Generator families were reorganized with stronger page semantics: open on current bank, playable transport, intentional `ResetGen`, explicit `Init Layer`/`Init Steps`/`Init Seq` split, and safer commit/cancel flow.
- `UI & Display`: Generator and step visuals were redesigned for readability (full `64-step` context + active `16-step` bank + playhead), with clearer layer graphics and lower redraw overhead (`30 fps`, skip unchanged frames). 
- `System & Defaults`: Settings/save flow was simplified and hardened (`System` save flow, save prompts, `Chaos Defaults` persistence, `Menu Wrap`, screensaver/wake refinement) with better memory posture versus upstream (`SRAM`/`CCRAM` reduction vs lineage firmwares). 
- `Simulators`: Desktop and Web simulator paths were clarified; Desktop gained configurable DIN/USB MIDI routing, tracing, better Launchpad port/model handling, and firmware-aligned behavior. 
- `Documentation`: Website, manual, cheatsheet, and simulator docs are maintained as a coherent layer aligned to current firmware behavior, including improved online manual search/navigation and a complete rework of the Launchpad cheatsheet. 
- `Ecosystem cleanup`: Ongoing coherence work across site, docs, simulator tooling, and reference pages.

Current validation scope:
- Real hardware testing is still useful for `Reset Pulse` / `Reset Gate`.
- Real hardware testing is still useful for `Voltage Mode` user scales in `Note`, `Arp`, and `Stochastic`. 
<br>  !! TO BE FIXED in ARP and STOCHASTIC !!
- Desktop Simulator USB MIDI has been validated well on `macOS / OS X` with `Launchpad Mini MK3`, but not yet across other Launchpads or operating systems.
- Hardware USB host support is limited to one directly connected controller at a time. USB hubs, including powered hubs, are unsupported; devices behind a hub are not enumerated by the current firmware.

For the broader curated overview, see <a href="https://vinxscorza.github.io/performer/features/" target="_blank" rel="noopener noreferrer">FEATURES</a>. For the exact technical chronology, including inherited upstream history, see the repository <a href="CHANGELOG.md" target="_blank" rel="noopener noreferrer">CHANGELOG</a>.

To clone this repository:

```bash
git clone https://github.com/VinxScorza/performer.git
cd performer
```

Then follow the standard build instructions for Westlicht Performer below.

--- ORIGINAL WESTLICHT DOCUMENTATION BELOW, WITH VINX ADDITIONS WHERE RELEVANT ---

# PER|FORMER

<a href="doc/sequencer.jpg" target="_blank" rel="noopener noreferrer"><img src="doc/sequencer.jpg"/></a>

## Overview

This repository contains the firmware for the **PER|FORMER** eurorack sequencer.

For more information on the project go <a href="https://westlicht.github.io/performer" target="_blank" rel="noopener noreferrer">here</a>.

The hardware design files are hosted in a separate repository <a href="https://github.com/westlicht/performer-hardware" target="_blank" rel="noopener noreferrer">here</a>.

## Development

If you want to do development on the firmware, the following is a quick guide on how to setup the development environment to get you going.

### Setup on macOS and Linux

First you have to clone this repository (make sure to add the `--recursive` option to also clone all the submodules):

```
git clone --recursive https://github.com/VinxScorza/performer.git
```

After cloning, enter the performer directory:

```
cd performer
```

### STM32 Toolchain Resolution (Vinx note)

For STM32 builds, the toolchain file now resolves `arm-none-eabi` tools in this order:

1. `tools/gcc-arm-none-eabi/bin` inside this repository
2. `TOOLCHAIN_ROOT` (CMake variable or environment variable, expected to point to the directory containing `arm-none-eabi-*`)
3. system `PATH`

This keeps clean builds deterministic across machines while still allowing explicit overrides.

Make sure you have a recent version of CMake installed. If you are on Linux, you might also want to install a few other packages. For Debian based systems, use:

```
sudo apt-get install libtool autoconf cmake libusb-1.0.0-dev libftdi-dev pkg-config
```

To compile for the hardware and allow flashing firmware you have to install the ARM toolchain and build OpenOCD:

```
make tools_install
```

Next, you have to setup the build directories:

```
make setup_stm32
```

If you also want to compile/run the simulator use:

```
make setup_sim
```

The simulator is great when developing new features. It allows for a faster development cycle and a better debugging experience.

### Setup on Windows

Currently, there is no native support for compiling the firmware on Windows. As a workaround, there is a Vagrantfile to allow setting up a Vagrant virtual machine running Linux for compiling the application.

First you have to clone this repository (make sure to add the `--recursive` option to also clone all the submodules):

```
git clone --recursive https://github.com/VinxScorza/performer.git
```

Next, go to <a href="https://www.vagrantup.com/downloads.html" target="_blank" rel="noopener noreferrer">https://www.vagrantup.com/downloads.html</a> and download the latest Vagrant release. Once installed, use the following to setup the Vagrant machine:

```
cd performer
vagrant up
```

This will take a while. When finished, you have a virtual machine ready to go. To open a shell, use the following:

```
vagrant ssh
```

When logged in, you can follow the development instructions below, everything is now just the same as with a native development environment on macOS or Linux. The only difference is that while you have access to all the source code on your local machine, you use the virtual machine for compiling the source.

To stop the virtual machine, log out of the shell and use:

```
vagrant halt
```

You can also remove the virtual machine using:

```
vagrant destroy
```

### Build directories

After successfully setting up the development environment you should now have a list of build directories found under `build/[stm32|sim]/[release|debug]`. The `release` targets are used for compiling releases (more code optimization, smaller binaries) whereas the `debug` targets are used for compiling debug releases (less code optimization, larger binaries, better debugging support).

### Developing for the hardware

You will typically use the `release` target when building for the hardware. So you first have to enter the release build directory:

```
cd build/stm32/release
```

To compile everything, simply use:

```
make -j
```

Using the `-j` option is generally a good idea as it enables parallel building for faster build times.

To compile individual applications, use the following make targets:

- `make -j sequencer` - Main sequencer application
- `make -j sequencer_standalone` - Main sequencer application running without bootloader
- `make -j bootloader` - Bootloader
- `make -j tester` - Hardware tester application
- `make -j tester_standalone` - Hardware tester application running without bootloader

Building a target generates a list of files. For example, after building the sequencer application you should find the following files in the `src/apps/sequencer` directory relative to the build directory:

- `sequencer` - ELF binary (containing debug symbols)
- `sequencer.bin` - Raw binary
- `sequencer.hex` - Intel HEX file (for flashing)
- `sequencer.srec` - Motorola SREC file (for flashing)
- `sequencer.list` - List file containing full disassembly
- `sequencer.map` - Map file containing section/offset information of each symbol
- `sequencer.size` - Size file containing size of each section

If compiling the sequencer, an additional `UPDATE.DAT` file is generated which can be used for flashing the firmware using the bootloader.

To simplify flashing an application to the hardware during development, each application has an associated `flash` target. For example, to flash the bootloader followed by the sequencer application use:

```
make -j flash_bootloader
make -j flash_sequencer
```

Flashing to the hardware is done using OpenOCD. By default, this expects an Olimex ARM-USB-OCD-H JTAG to be attached to the USB port. You can easily reconfigure this to use a different JTAG by editing the `OPENOCD_INTERFACE` variable in the `src/platform/stm32/CMakeLists.txt` file. Make sure to change both occurrences. A list of available interfaces can be found in the `tools/openocd/share/openocd/scripts/interface` directory (or `/home/vagrant/tools/openocd/share/openocd/scripts/interface` when running the virtual machine).

### Developing for the simulator

Note that the simulator is only supported on macOS and Linux and does not currently run in the virtual machine required on Windows.

You will typically use the `debug` target when building for the simulator. So you first have to enter the debug build directory:

```
cd build/sim/debug
```

To compile everything, simply use:

```
make -j
```

To run the simulator, use the following:

```
./src/apps/sequencer/sequencer
```

Note that you have to start the simulator from the build directory in order for it to find all the assets.

Desktop simulator MIDI can be configured from the command line. Use `./src/apps/sequencer/sequencer --midi` to list available ports, then launch with options such as:

```
./src/apps/sequencer/sequencer --midi-port "Your MIDI Port"
./src/apps/sequencer/sequencer --midi-in "Your Keyboard In" --midi-out "Your Synth Out"
./src/apps/sequencer/sequencer --usb-midi-port "Your Launchpad Port"
```

By default, the Desktop Simulator now starts with both DIN MIDI and USB MIDI disabled, so it does not try to open missing macOS MIDI devices unless you explicitly assign them.
Use `--trace-midi` to inspect incoming simulator MIDI messages and `--trace-dio` if you want terminal output for the simulated `CLK OUT` and `RESET OUT` digital lines.

Use `none`, `off`, or `-` to disable a default input or output assignment, for example:

```
./src/apps/sequencer/sequencer --midi-out none
```

On macOS, some devices expose port directions from the device point of view instead of the simulator point of view. For example, a Launchpad Mini MK3 on the simulated USB MIDI port may need:

```
./src/apps/sequencer/sequencer --usb-midi-in "Launchpad Mini MK3 LPMiniMK3 DAW Out" --usb-midi-out "Launchpad Mini MK3 LPMiniMK3 DAW In"
```

In the current Vinx setup, the simulator-side USB MIDI workflow has only been validated on macOS / OS X with a `Launchpad Mini MK3`, since that is the only Launchpad currently available for local testing.
The simulator now also infers the Launchpad model from the selected USB port names, so a Mini MK3 is no longer exposed to the firmware as the old Mini Mk2 default device.
For Launchpad-style USB controllers, the simulator also mirrors both the matching sibling input and output ports (`DAW` / `MIDI`) when possible, so pad presses and initialization messages can still land on the endpoint the device actually uses.

### Source code directory structure

The following is a quick overview of the source code directory structure:

- `src` - Top level source directory
- `src/apps` - Applications
- `src/apps/bootloader` - Bootloader application
- `src/apps/hwconfig` - Hardware configuration files
- `src/apps/sequencer` - Main sequencer application
- `src/apps/tester` - Hardware tester application
- `src/core` - Core library used by both the sequencer and hardware tester application
- `src/libs` - Third party libraries
- `src/os` - Shared OS helpers
- `src/platform` - Platform abstractions
- `src/platform/sim` - Simulator platform
- `src/platform/stm32` - STM32 platform
- `src/test` - Test infrastructure
- `src/tests` - Unit and integration tests

The two platforms both have a common subdirectories:

- `drivers` - Device drivers
- `libs` - Third party libraries
- `os` - OS abstraction layer
- `test` - Test runners

The main sequencer application has the following structure:

- `asteroids` - Asteroids game (disabled in active builds)
- `engine` - Engine responsible for running the sequencer core
- `model` - Data model storing the live state of the sequencer and many methods to change that state
- `python` - Python bindings for running tests using python
- `tests` - Python based tests
- `ui` - User interface

## Third Party Libraries

The following third party libraries are used in this project.

- <a href="http://www.freertos.org" target="_blank" rel="noopener noreferrer">FreeRTOS</a>
- <a href="https://github.com/libopencm3/libopencm3" target="_blank" rel="noopener noreferrer">libopencm3</a>
- <a href="https://github.com/libusbhost/libusbhost" target="_blank" rel="noopener noreferrer">libusbhost</a>
- <a href="https://github.com/memononen/nanovg" target="_blank" rel="noopener noreferrer">NanoVG</a>
- <a href="http://elm-chan.org/fsw/ff/00index_e.html" target="_blank" rel="noopener noreferrer">FatFs</a>
- <a href="https://github.com/nothings/stb/blob/master/stb_sprintf.h" target="_blank" rel="noopener noreferrer">stb_sprintf</a>
- <a href="https://github.com/nothings/stb/blob/master/stb_image_write.h" target="_blank" rel="noopener noreferrer">stb_image_write</a>
- <a href="https://sol.gfxile.net/soloud/" target="_blank" rel="noopener noreferrer">soloud</a>
- <a href="https://www.music.mcgill.ca/~gary/rtmidi/" target="_blank" rel="noopener noreferrer">RtMidi</a>
- <a href="https://github.com/pybind/pybind11" target="_blank" rel="noopener noreferrer">pybind11</a>
- <a href="https://github.com/c42f/tinyformat" target="_blank" rel="noopener noreferrer">tinyformat</a>
- <a href="https://github.com/Taywee/args" target="_blank" rel="noopener noreferrer">args</a>

## License

<a href="https://opensource.org/licenses/MIT" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License: MIT"></a>

This work is licensed under a <a href="https://opensource.org/licenses/MIT" target="_blank" rel="noopener noreferrer">MIT License</a>.
