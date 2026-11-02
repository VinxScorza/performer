# PER|FORMER — direct upgrade smoke test: v0.4.4 → v0.4.6-beta.2

Estimated time: 25–35 minutes, plus an optional Launchpad check. Hardware result: **NOT YET RUN**.
Start with v0.4.4 firmware and projects saved by that version. This is a direct upgrade to beta.2; v0.4.5 does not need to be installed in between. The installed firmware displays `0.4.6`; beta status belongs to the release tag and package. The checklist also covers changes introduced in v0.4.5, which remains the stable version.

> **IMPORTANT — project compatibility:** the 0.4.6 beta opens v0.4.4 and v0.4.5 projects, but saving upgrades them to `Version41`. Firmware v0.4.5 and earlier cannot reopen an upgraded file. Back up the entire SD card or keep a separate copy of every original project before testing.

## Baseline on v0.4.4

- [ ] Confirm `0.4.4` on the System page. Save a reference project with at least Note, Curve, Arp, and Stochastic tracks, two patterns, a CV/CC route, and recognizable scale/root settings. Copy the SD card before upgrading.
- [ ] Record BPM, clock source, divisors, First/Last Step, scale/root, mutes, and MIDI configuration. Record or note a short playback of deterministic tracks. For random tracks, compare scale, register, and settings rather than expecting identical note sequences.
- [ ] Check Play/Stop, pattern changes, entry to Note/System, and the Load list. Use these observations to spot regressions after upgrading.

## Installation and startup

- [ ] Put the `v0.4.6-beta.2` package's `UPDATE.DAT` in the SD card root.
- [ ] Power on while holding the encoder, or open System → Update and hold the encoder. The bootloader should recognize version `0.4.6` and verify the file. Confirm YES and wait for completion without removing power.
- [ ] Confirm `0.4.6` on the System page; check the display and buttons and watch for unexpected restarts.
- [ ] Try startup with a separate test SD card, then with a copy of the usual card. If the module remains on `LOADING`, do not alter the original card. Record which copy hangs, then retry on the copy after renaming `LAST.DAT`. This follows the still-open report in [#22](https://github.com/VinxScorza/performer/issues/22).
- [ ] Load the copied v0.4.4 project. Check patterns, tracks, routing, BPM, divisors, and MIDI settings. All new MIDI filters should initially be enabled. Start and stop playback, check CV/gates against the baseline, and review Arp/Stochastic scale behavior below.

## Changes since v0.4.4, including v0.4.5

- [ ] On a Note track, open Acid → Eucl Phrase. Generate a phrase, vary the Euclidean parameters, and compare original/preview with A/B. Cancel should restore the original steps; Apply should keep a newly generated phrase. If available, repeat from Launchpad Generators Mode.
- [ ] On a pattern copy, open Chaos → Wreck Pattern, generate, try A/B and Cancel. Repeat with Apply, then visit Note and System. Check for restarts, hangs, or changes left behind by Cancel. Repeat the cycle at least five times to catch obvious stability problems.
- [ ] Test Routing with one continuous target (such as tempo), one discrete target (pattern or divisor), and one boolean target (mute/fill). Expect smooth movement, valid steps, and stable on/off behavior respectively. Try a Curve-only target on a Note track: the incompatible route should be rejected or ignored without corrupting the track.
- [ ] Browse Load with the usual SD-card copy, load two projects, and try Save As. Check for restarts or hangs and keep the original v0.4.4 project untouched.

## Editing and navigation

- [ ] On Note, create four recognizable steps, select them, and use Copy. The source selection should clear. Paste without a destination should show `SELECT DEST` and leave the pattern unchanged. Select four destination steps and paste; they should match the source.
- [ ] Select steps 1–4 and use Duplicate: the copy should land on 5–8, the selection should move to 5–8, and Last Step should extend if needed. Also duplicate a non-contiguous selection; its gaps should be preserved.
- [ ] Select only the last available step and duplicate: expect `NO ROOM` and no data change. With no selection, check the usual First/Last range duplication.
- [ ] Repeat Copy/Paste and Duplicate at least once on Curve, Logic, Stochastic, and Arp.
- [ ] On the Track page, double-click a track key and check that the correct editor opens. On the Project page, a single track-key press should open Steps.

## Temporary Fill divisor

- [ ] Set a Note track to a regular rhythm with a known divisor. On the Performer page, hold S9 (Fill T1) and turn the encoder: the track rate should change, not the global BPM. Release S9: the original divisor should return. Check the stored value on the Sequence page.
- [ ] Hold S1 and turn the encoder: it should change T1 Fill Amount, not divisor or BPM.
- [ ] Try global Fill and the temporary rate change on independent Curve, Logic, Stochastic, and Arp tracks, with no track links. Check for hangs or stuck gates after Fill ends.
- [ ] On the Performer page, latch T1 Fill with `SHIFT + S9`. Turn the encoder without holding the button: its temporary divisor should change. Latch T2: the encoder should follow T2. Latch global Fill with `SHIFT + F4`: the encoder should affect all active Fill tracks. Unlatch and check that stored divisors return.
- [ ] If available, start a track Fill from Launchpad and turn the module encoder; check the temporary change and restoration on release. Disconnect the controller after release and verify normal encoder use.

## MIDI and scales

- [ ] Connect a MIDI source to the configured port and channel. With MIDI Notes enabled, hold a note, disable the filter, then release the note: Note Off should still close the note/gate. With the filter disabled, new Note On messages should be blocked; re-enable it and retry.
- [ ] In the Note editor, select all steps with double `SHIFT` and send MIDI Note Trigs as reported in issue #22. With `MIDI Notes = No`, selected pitches and gates should remain unchanged. Re-enable Notes and check that MIDI note entry works again.
- [ ] With a CC assigned to an obvious route, check that `MIDI CC = No` blocks its effect and `Yes` restores it. Where suitable destinations are configured, test Program Change, Pitch Bend, and Aftertouch in the same way.
- [ ] Save the project with some filters disabled, reload it, and check that the filter values persist.
- [ ] On Arp and Stochastic, try C minor pentatonic with legacy/bypass chromatic pitch positions: E-flat should remain E-flat, and C-sharp should be excluded from the note selection. Listen or measure pitch, then check Semitones mode.
- [ ] Route CV or CC to Divisor, move it slowly, and hold near a division boundary. Divisions should remain valid and should not repeatedly switch with small input fluctuations.

## Final checks

- [ ] Play for at least two minutes while changing patterns and using Fill. Check for crashes, restarts, or unexpected stops.
- [ ] Save under a new name, restart, and reload. Notes, routing, and MIDI filters should persist; the temporary Fill divisor should not change the stored sequence divisor.

For any failure, record track type, pattern, internal/external clock, connected controllers, the exact key sequence, and the observed result. Passing this checklist is a smoke test, not complete validation of every feature.
