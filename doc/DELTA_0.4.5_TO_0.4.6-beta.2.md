# PER|FORMER — operational delta and smoke test: v0.4.5 → v0.4.6-beta.2

Validation status: **local builds, automated tests, and CI passed; the two beta.2 changes have not yet been tested on the module**. The startup hang with an existing SD card reported in [#22](https://github.com/VinxScorza/performer/issues/22) is still under investigation.

The baseline is the `0.4.5` tag. For firmware comparison, that tag has the same sources as release commit `cfc19cf9`; the following commit changed only presentation and licensing.

> **IMPORTANT — back up the SD card before testing this beta.** Beta.2 can open v0.4.5 projects, but saving converts them from `Version40` to `Version41`. Firmware v0.4.5 and earlier then reject the upgraded file during its integrity check and cannot load it. Keep an untouched copy of each original v0.4.5 project, separate from the copy used with the beta.

## Functional changes

| Area | Change from v0.4.5 | Verification completed | Check on the module |
|---|---|---|---|
| Projects | The format moves to `Version41` to store MIDI input filters. `Version40` projects load with all new filters enabled. | Load and initialization paths reviewed; STM32 build passed. | Open a real v0.4.5 project, save a copy under a new name, and reload it. |
| MIDI input | New filters for Notes, CC, Program Change, Pitch Bend, and Aftertouch. Beta.2 also prevents filtered Note On messages from editing selected Note steps. Note Off remains accepted; Program Change mapping remains fixed. | UI test for selected-step editing and automated tests for filtering and Note On/Off; CC and Program Change confirmed on hardware with beta.1. | Repeat the Note Trig/selected-step case with `MIDI Notes = No`; test Bend, Aftertouch, and saved filter settings. |
| Copy/Paste/Duplicate | Selection-aware behavior across Note, Curve, Stochastic, Logic, and Arp. Paste requires an explicit destination; Duplicate preserves selection gaps, extends Last Step, and reports `NO ROOM`. | C++ tests; overall behavior confirmed on hardware with beta.1. | Optionally check each editor not covered in the hardware report. |
| Navigation | Double-clicking `T1..T8` opens the selected track's Steps editor when the active page does not reserve those keys. | UI tests and beta.1 hardware confirmation. | No beta.2-specific check required. |
| Fill | The encoder temporarily changes playback division during Fill without changing the stored divisor. Beta.2 also works without holding the button after Fill is latched. With multiple latches, the most recently latched track has priority; a global latch selected last affects all active Fill tracks. | UI tests for priority and restoration; momentary, latched, and global Fill confirmed on hardware with beta.1. | Test the encoder with one latch, multiple latches, global Fill, and restoration after unlatching. |
| Linked tracks | As in v0.4.5, a linked track inherits timing and divisor from its source track. The Fill divisor override affects independent tracks and does not replace an inherited divisor. | Playback paths reviewed and behavior documented in the manual. | Compare one independent track with one linked track. |
| Routing Divisor | Min, Max, and modulation move through valid musical divisions; routed values are quantized and stabilized near boundaries. | C++ tests; confirmed on hardware with an Octatrack CC in beta.1. | Optionally test CV near a division boundary. |
| Stochastic/Arp scale | Legacy chromatic pitch slots are interpreted consistently after reloading a project with a non-chromatic scale; the related Arp path follows the same rule. | C++ tests; Stochastic and Arp behavior confirmed on hardware with beta.1. | Optionally test migration with a real v0.4.5 project. |
| Vagrant build | Provisioning stops on error, verifies the CMake installer SHA-256, and installs compatible Python dependencies. | Contributor confirmed a clean VM build through `UPDATE.DAT` generation. | No module-specific check required. |

## Checks completed

- Desktop simulator build and 53 UI tests.
- Thirteen C++ test executables.
- STM32 Release build and `UPDATE.DAT` generation.
- WebAssembly build and browser loading with a working display and no console errors.
- Firmware checksum, display version `0.4.6`, flash limit, and initial vectors.
- Website link/version check distinguishing stable `v0.4.5` from beta `v0.4.6-beta.2`.
- All five CI jobs for the release tag: docs, STM32, Linux simulator, macOS simulator, and WebAssembly.

These checks cover logic, compilation, and integration. They do not replace testing CV, gates, MIDI, encoder, buttons, and SD-card behavior on a physical Performer.

## Focused smoke test from v0.4.5

Estimated time: 15–20 minutes, plus a short optional Launchpad check.

### 1. Prepare

- [ ] On v0.4.5, open a project with at least a Note, Curve, Stochastic, or Arp track; a Divisor route; and recognizable MIDI settings. Prepare a separate test SD card: issue #22 reports a startup hang with an existing card.
- [ ] Back up the entire SD card or make a separate copy of the original project. Do not use your only copy for this test.
- [ ] Record the BPM, divisors, scale/root, active pattern, and basic CV/gate behavior.

### 2. Update and check compatibility

- [ ] Install `UPDATE.DAT` from `v0.4.6-beta.2` and confirm that the module displays `0.4.6`.
- [ ] Test startup with the separate test card first, then with a copy of the usual card. If it hangs on `LOADING`, leave the original untouched; record which copy hangs and retry on the copy after renaming `LAST.DAT`.
- [ ] Open the copied v0.4.5 project. Check tracks, patterns, BPM, routing, scales, and outputs.
- [ ] On the Project page, confirm that MIDI Notes, CC, Pgm, Bend, and After initially read `Yes`.

### 3. Changed functions

- [ ] Copy a few Note steps. Paste without selecting a destination should show `SELECT DEST`; selecting destination steps should then paste correctly.
- [ ] Duplicate a continuous selection and one with gaps. The new selection should move to the copy, Last Step should extend when needed, and an attempt with no room should show `NO ROOM` without changing data. Repeat briefly in Curve, Logic, Stochastic, and Arp.
- [ ] On the Track page, double-click a track key and confirm that the correct Steps editor opens.
- [ ] On the Performer page, hold Fill for an independent track and turn the encoder. Only the temporary divisor should change; the original rhythm and stored divisor should return on release. Fill Amount must still work.
- [ ] Latch Fill with `SHIFT + S9`, then turn the encoder without holding the button: T1's temporary divisor should change. Latch another track: the encoder should follow the latest latch. Latch global Fill with `SHIFT + F4`: the encoder should affect all active Fill tracks. Unlatch and confirm restoration.
- [ ] Repeat with a linked track: it should continue to inherit timing and divisor from its source track.
- [ ] Move a Divisor route slowly across a division boundary. Only valid divisions should appear, without repeated switching near the boundary.
- [ ] Reload a project with Stochastic or Arp on a non-chromatic scale and check that notes remain in the expected scale and register.

### 4. MIDI and saving

- [ ] Hold a MIDI note, set MIDI Notes to `No`, and release it: Note Off should close the gate. New Note On messages should be blocked.
- [ ] Open the Note editor, select all steps with double `SHIFT`, and send MIDI Note Trigs. With `MIDI Notes = No`, selected pitches and gates should stay unchanged; with `Yes`, normal MIDI note entry should work again.
- [ ] Test at least one CC assigned to a route. If available, toggle `Yes`/`No` for Program Change, Pitch Bend, and Aftertouch with suitable destinations.
- [ ] Save **the test copy under a new name**, restart, and reload it. MIDI filters and edits should persist; the temporary Fill divisor should not be saved.

### 5. Final check

- [ ] Let the project play for at least two minutes while changing patterns and using Fill. Check for hangs, restarts, and stuck gates.
- [ ] Keep the original v0.4.5 project untouched. If returning to stable firmware, load that original copy, not a project saved by the beta.

## Memory compared with v0.4.5

Both local versions were rebuilt with the same GNU ARM 6.3.1 toolchain.

| Region | Free in v0.4.5 | Free in beta.2 local build | Change in free space |
|---|---:|---:|---:|
| Application flash | 242216 B | 223004 B | −19212 B |
| SRAM after static data | 16244 B | 16228 B | **−16 B** |
| CCMRAM after static data | 14768 B | 14756 B | −12 B |

The local beta.2 build therefore has **16 fewer bytes of free static SRAM** than v0.4.5. Compared with the local beta.1 build, SRAM is unchanged, free CCMRAM is down by 12 bytes, and free flash is down by 2012 bytes. The CI-distributed beta.2 firmware leaves 222684 B of application flash, 320 B less than the local beta.2 build; CI SRAM/CCMRAM static sections were not measured directly. These figures do not measure peak runtime stack or heap use, which needs a check on the module.

## Outcome

Beta.2 is published as a prerelease. Local checks and CI passed, but the two new behaviors still need physical hardware testing. The SD-card startup report in #22 remains open. Keep the pre-beta project copy untouched before any save in `Version41`.
