# Vinx PER|FORMER firmware — v0.4.6

Firmware v0.4.6 brings the editing, MIDI, Fill, Routing Divisor, and scale improvements developed through the beta releases. The module displays `0.4.6`.

> **Important project compatibility warning:** v0.4.6 opens projects saved by v0.4.4 and v0.4.5, but saving upgrades them to `Version41`. Firmware v0.4.5 and earlier cannot reopen the upgraded files. Back up the SD card or keep a separate copy of each original project before saving with v0.4.6.

## Changes since v0.4.5

- **MIDI input filters:** each project can filter Notes, CC, Program Change, Pitch Bend, and Aftertouch independently. Clock and Transport are unaffected. With Notes disabled, Note On is blocked in both the engine and Note step editor; Note Off remains accepted so active notes can be released. Custom Program Change mapping is not included.
- **Copy, Paste, and Duplicate:** selected-step operations are consistent across Note, Curve, Stochastic, Logic, and Arp. Paste requires a selected destination; Duplicate preserves selection gaps, moves the selection to the copy, extends Last Step when needed, and reports when there is no room.
- **Track navigation:** double-clicking T1–T8 opens that track's Steps editor where the active page does not reserve those buttons.
- **Fill divisor:** while holding Fill, turn the module encoder to change the temporary playback divisor without changing the saved divisor. The control also works from the Launchpad Fill workflow. On the Performer page, a latched Fill can be edited with the encoder alone; the most recently latched track has priority, and a global latch selected last affects all active Fill tracks. Linked tracks continue to inherit their source track's timing.
- **Routing Divisor:** Min, Max, and routed modulation now use valid musical divisions with stabilization near boundaries.
- **Stochastic and Arp scales:** correct legacy pitch-slot interpretation after reloading a project that uses a non-chromatic scale.
- **Build and documentation:** Vagrant provisioning now stops on errors and verifies the CMake installer checksum. The simulators, site, manual, printable PDF, and smoke-test guides have been updated.

## Validation and open report

Automated UI/C++ tests, STM32 and WebAssembly builds, CI, and browser simulator loading passed. Earlier beta hardware feedback confirmed the main editing, Routing Divisor, MIDI CC/Program Change filter, Fill, and scale behavior. Dedicated hardware checks of the MIDI Notes step-editor fix and one-handed latched Fill control remain pending.

A startup hang reported with one existing SD card in [issue #22](https://github.com/VinxScorza/performer/issues/22) remains under investigation. It has not been reproduced or diagnosed here. Keep the original card backed up and report any recurrence.

For a focused upgrade check, use the [v0.4.5 → v0.4.6 smoke test](https://github.com/VinxScorza/performer/blob/master/doc/DELTA_0.4.5_TO_0.4.6.md). For a direct upgrade from v0.4.4, use the [full smoke test](https://github.com/VinxScorza/performer/blob/master/doc/SMOKE_TEST_0.4.6.md).
