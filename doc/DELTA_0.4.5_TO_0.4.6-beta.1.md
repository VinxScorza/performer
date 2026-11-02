# PER|FORMER — operational delta and smoke test: v0.4.5 → v0.4.6-beta.1

Historical validation snapshot at beta.1 publication: **automated checks passed; the final module smoke test was still pending**. Later hardware feedback is recorded in [issue #22](https://github.com/VinxScorza/performer/issues/22).

The baseline is the `0.4.5` tag. For firmware comparison, that tag has the same sources as release commit `cfc19cf9`; the following commit changed only presentation and licensing.

> **IMPORTANT — back up the SD card before testing this beta.** Beta.1 can open v0.4.5 projects, but saving converts them from `Version40` to `Version41`. Firmware v0.4.5 and earlier then reject the upgraded file during its integrity check and cannot load it. Keep an untouched copy of each original v0.4.5 project, separate from the copy used with the beta.

## Functional changes

| Area | Change from v0.4.5 | Verification completed at beta.1 release | Check on the module |
|---|---|---|---|
| Projects | The format moves to `Version41` to store MIDI input filters. `Version40` projects load with all new filters enabled. | Load and initialization paths reviewed; STM32 build passed. | Open a real v0.4.5 project, save a copy under a new name, and reload it. |
| MIDI input | New filters for Notes, CC, Program Change, Pitch Bend, and Aftertouch. Note Off remains accepted even when Notes is disabled. Program Change mapping remains fixed as in v0.4.5. | Automated checks for defaults, CC filtering, and Note On/Note Off. | Check real MIDI and USB MIDI ports, Program Change, Bend, Aftertouch, and persistence after saving. |
| Copy/Paste/Duplicate | Selection-aware behavior across Note, Curve, Stochastic, Logic, and Arp. Paste requires an explicit destination; Duplicate preserves selection gaps, extends Last Step, and reports `NO ROOM`. | C++ tests for continuous and gapped selections, no-room handling, and Paste without a destination; all five editors compiled. | Make a quick manual pass in each editor. |
| Navigation | Double-clicking `T1..T8` opens the selected track's Steps editor when the active page does not reserve those keys. | UI tests for double-click and Project-page navigation. | Check double-click sensitivity on physical buttons. |
| Fill | During Fill, the encoder can temporarily change the track divisor without changing the stored divisor or BPM. It works from the panel and Launchpad. | UI tests for direct, global, modal, and Launchpad controls; override clearing on release and disconnection. | Listen for the rate change on Note, Curve, Logic, Stochastic, and Arp. |
| Linked tracks | As in v0.4.5, a linked track inherits timing and divisor from its source track. The Fill divisor override affects independent tracks and does not replace an inherited divisor. | Playback paths reviewed and behavior documented in the manual. | Compare one independent track with one linked track. |
| Routing Divisor | Min, Max, and modulation move through valid musical divisions; routed values are quantized and stabilized near boundaries. | C++ tests for Min/Max, quantization, sequence write, and hysteresis. | Move CV or CC slowly near a boundary and check for audible or visible oscillation. |
| Stochastic/Arp scale | Legacy chromatic pitch slots are interpreted consistently after reloading a project with a non-chromatic scale; the related Arp path follows the same rule. | C++ tests for minor pentatonic, in-scale and out-of-scale notes, Semitones, transposition, and custom Voltage scales. | Check pitch on a reloaded v0.4.5 project. |
| Vagrant build | Provisioning stops on error, verifies the CMake installer SHA-256, and installs compatible Python dependencies. | Contributor confirmed a clean VM build through `UPDATE.DAT` generation. | No module-specific check required. |

## Checks completed at beta.1 release

- Desktop simulator build and 47 UI tests.
- Thirteen C++ test executables.
- STM32 Release build and `UPDATE.DAT` generation.
- WebAssembly build and browser loading of the web simulator.
- Firmware checksum, display version `0.4.6`, flash limit, and initial vectors.
- Website link/version check distinguishing stable `v0.4.5` from beta `v0.4.6-beta.1`.

These checks cover logic, compilation, and integration. They do not replace testing CV, gates, MIDI, encoder, buttons, and SD-card behavior on a physical Performer.

## Focused smoke test from v0.4.5

Estimated time: 15–20 minutes, plus a short optional Launchpad check.

### 1. Prepare

- [ ] On v0.4.5, open a project with at least a Note, Curve, Stochastic, or Arp track; a Divisor route; and recognizable MIDI settings.
- [ ] Back up the entire SD card or make a separate copy of the original project. Do not use your only copy for this test.
- [ ] Record the BPM, divisors, scale/root, active pattern, and basic CV/gate behavior.

### 2. Update and check compatibility

- [ ] Install `UPDATE.DAT` from `v0.4.6-beta.1` and confirm that the module displays `0.4.6`.
- [ ] Open the copied v0.4.5 project. Check tracks, patterns, BPM, routing, scales, and outputs.
- [ ] On the Project page, confirm that MIDI Notes, CC, Pgm, Bend, and After initially read `Yes`.

### 3. Changed functions

- [ ] Copy a few Note steps. Paste without selecting a destination should show `SELECT DEST`; selecting destination steps should then paste correctly.
- [ ] Duplicate a continuous selection and one with gaps. The new selection should move to the copy, and an attempt on the last step should show `NO ROOM` without changing data. Repeat briefly in Curve, Logic, Stochastic, and Arp.
- [ ] On the Track page, double-click a track key and confirm that the correct Steps editor opens.
- [ ] On the Performer page, hold Fill for an independent track and turn the encoder. Only the temporary divisor should change; the original rhythm and stored divisor should return on release. Fill Amount must still work.
- [ ] Repeat with a linked track: it should continue to inherit timing and divisor from its source track.
- [ ] Move a Divisor route slowly across a division boundary. Only valid divisions should appear, without repeated switching near the boundary.
- [ ] Reload a project with Stochastic or Arp on a non-chromatic scale and check that notes remain in the expected scale and register.

### 4. MIDI and saving

- [ ] Hold a MIDI note, set MIDI Notes to `No`, and release it: Note Off should close the gate. New Note On messages should be blocked.
- [ ] Test at least one CC assigned to a route. If available, toggle `Yes`/`No` for Program Change, Pitch Bend, and Aftertouch.
- [ ] Save **the test copy under a new name**, restart, and reload it. MIDI filters and edits should persist; the temporary Fill divisor should not be saved.

### 5. Final check

- [ ] Let the project play for at least two minutes while changing patterns and using Fill. Check for hangs, restarts, and stuck gates.
- [ ] Keep the original v0.4.5 project untouched. If returning to stable firmware, load that original copy, not a project saved by the beta.

## Memory compared with v0.4.5

Both local versions were rebuilt with the same GNU ARM 6.3.1 toolchain.

| Region | Free in v0.4.5 | Free in beta.1 local build | Change in free space |
|---|---:|---:|---:|
| Application flash | 242216 B | 225016 B | −17200 B |
| SRAM after static data | 16244 B | 16228 B | **−16 B** |
| CCMRAM after static data | 14768 B | 14768 B | 0 B |

The beta.1 local build has **16 fewer bytes of free static SRAM** than v0.4.5, leaving 16228 B of SRAM and 14768 B of CCMRAM in their separate regions. These figures do not measure peak runtime stack or heap use, which needs a check on the module.

## Outcome at beta.1 release

The software delta matched the changelog and manual. No unexpected operational incompatibility was found beyond the inability of older firmware to read a project saved in `Version41`. Beta.1 was published for hardware smoke testing; dedicated validation was still pending at release time. See [issue #22](https://github.com/VinxScorza/performer/issues/22) for subsequent tester feedback and the remaining open items.
