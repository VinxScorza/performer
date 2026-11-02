# Vinx PER|FORMER firmware — v0.4.6-beta.2

This is a beta release. **v0.4.5 remains the stable version.** The module displays firmware version `0.4.6`; the beta status is indicated by the release tag.

> **Project compatibility warning:** the 0.4.6 beta can open projects saved by v0.4.4 and v0.4.5, but saving upgrades them to `Version41`. Firmware v0.4.5 and earlier cannot reopen those upgraded files. Back up the SD card or keep a separate copy of each original project before saving with this beta.

## Changes since beta.1

- **MIDI Notes filter in the Note editor:** with `MIDI Notes = No`, incoming Note On messages can no longer change selected pitches or gates. Note Off remains accepted so active notes can be released. This addresses the editor behavior reported in [issue #22](https://github.com/VinxScorza/performer/issues/22).
- **One-handed latched Fill divisor:** on the Performer page, after latching Fill with `SHIFT + S9–S16`, turn the encoder without holding the Fill button to change that track's temporary playback divisor. With several latched Fills, the most recently latched track has priority. If global Fill was latched last, the encoder adjusts all active Fill tracks. Holding a Fill control still takes priority, and unlatching restores the saved divisor.

Beta.2 also includes the beta.1 changes: project MIDI input filters, safer Copy/Paste/Duplicate, the track-button double-click shortcut, temporary Fill divisor control, valid Routing Divisor limits, and Stochastic/Arp scale handling after project reload. See the [changelog](https://github.com/VinxScorza/performer/blob/master/CHANGELOG.md) for details.

## Testing status

The desktop UI/C++ tests, local STM32 and WebAssembly builds, and browser simulator loading passed. The two beta.2 fixes still need a hardware check. The SD-card startup hang reported in [issue #22](https://github.com/VinxScorza/performer/issues/22) remains open; it has not been diagnosed or fixed in this release. Use a backed-up or spare SD card for testing.

For a focused upgrade check, use the [v0.4.5 → beta.2 smoke test](https://github.com/VinxScorza/performer/blob/master/doc/DELTA_0.4.5_TO_0.4.6-beta.2.md). For a direct upgrade from v0.4.4, use the [full smoke test](https://github.com/VinxScorza/performer/blob/master/doc/SMOKE_TEST_0.4.6.md).
