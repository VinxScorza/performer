# Changelog

## Vinx Scorza fork

<sub>Starting from `v0.3.2-vinx.1` (16 March 2026), this changelog includes changes specific to the Vinx Scorza fork. All entries below `v0.3.2-vinx.1` are inherited from the Mebitek fork history and are kept here as upstream reference. From the first standalone Vinx release onward, Vinx uses standalone semantic versioning (`v0.x.y`) while preserving the earlier `v0.3.2-vinx.*` entries as historical lineage. I try to preserve backward compatibility with older projects, settings, and workflows where possible, but I do not guarantee it for Vinx-specific changes.</sub>

# v0.4.1 (in progress)
- Versioning transition: this release drops the inherited `v0.3.2-vinx.*` naming and starts standalone Vinx semantic versioning (`v0.x.y`)
- Memory footprint improved versus upstream baseline: static SRAM and CCRAM usage are significantly reduced compared to Mebitek 0.3.2
- Launchpad refactor: complete the P1 structural split of Launchpad controller domains (high-risk area hardening)
- Add experimental `16-step Editing Mode` (Launch Control XL + BeatStep Pro mapping profile): explicit armed/disarmed entry, forced visible 16-step loop while armed, bank navigation, and loop-range restore on exit
- Add `16-step Editing Mode` controller feedback scope on LCXL/BSP profiles: gate-pad LEDs plus prev/next function-button feedback (knob-row LED remains device-local)
- Fix Performer `GEN` menu flow/labels by track type: on `Note` use explicit `Acid (Layer/Phrase)` and `Chaos (Vandalize/Wreck)` labels; on non-Note tracks expose `Chaos (Entropy)` (`ChaosEntropy` mode)
- Fix `Voltage Mode` behavior on `Arp` and `Stochastic` tracks so non-chromatic user scales are respected in bypass-scale paths instead of forcing semitone/chromatic fallback
- Extend Launchpad `Generators Mode` beyond Note tracks: keep the full Note map (`GRID 1/2/10/3/11/4/8/16` = `Random`, `Acid Layer`, `Acid Phrase`, `Vandalize`, `Wreck`, `Euclidean`, `Init Layer`, `Init Steps`) and add the dedicated non-Note subset on `Curve` / `Stochastic` / `Logic` / `Arp` (`GRID 1/3/4/8/16` = `Random`, `Entropy`, `Euclidean`, `Init Layer`, `Init Steps`)
- Add the new Chaos generator mode `Entropy` for non-Note Launchpad workflows, with Chaos-style interaction, dedicated target matrix, and persisted defaults in `System -> Chaos Defaults`
- Set `Slide Time` default to `10%` across track families (from the previous `20%` line default)
- Make user scales use dynamic names in all `Scale` menus; default user-scale slot names are now `INIT1`..`INIT4`
- Fix generator-selector track locking end-to-end: while the machine is inside the generator selector path (level 1 and deeper), Launchpad track/scene retarget is blocked so selection stays bound to the original track
- Fix generator-page retarget regressions by keeping Launchpad track/scene switching locked while generator pages are active
- From `Project`, `Layout`, `Routing`, `MIDI Output`, `User Scale`, and `Clock`, make track selection (`T1..8` on machine and `TRK1..8` on Launchpad) jump directly to `Steps` on the selected track
- Fix Launchpad Generators Mode UX regressions on hardware: stabilize overlay enter/exit behavior, keep selector mapping aligned
- Make `Init Steps` from Launchpad `Generators Mode` (`GRID 16`) apply immediately and exit the mode to plain `Steps`
- Add `TOP 7 + TOP 8` Launchpad undo shortcut on `Note` / `Curve` / `Logic` step-edit pages and make it a 1-level `Undo/Redo` toggle (same behavior as machine `PAGE + S7`)
- Make Generator commit from machine controls exit Launchpad `Generators Mode` cleanly to plain `Steps`, instead of returning to the LP overlay state
- Extend simulator regression coverage for Launchpad generator locking and selector flows, including explicit scene-switch lock checks for machine selector and generator pages and non-Note `Generators Mode` mapping/Init flows
- Harden STM32 clean-build reproducibility by resolving the ARM toolchain in a deterministic order (`repo tools` -> `TOOLCHAIN_ROOT` -> `PATH`) and by fixing an explicit `<algorithm>` include for `ClockTimer` (`std::min`) so from-scratch builds no longer depend on cache/toolchain accidents

# v0.3.2-vinx.1.5.2 (3 April 2026)
- Swap `Acid` mode order to `Layer` then `Phrase` in the machine `Acid` selector, and mirror that same swap in Launchpad `Generators Mode` (`GRID 2 = Acid Layer`, `GRID 10 = Acid Phrase`)
- Add `GRID 16 = Undo` to Launchpad `Generators Mode` (mirrors machine `PAGE + S7`): on generator preview pages it first reverts the current preview in its active scope (including `Wreck Pattern`), and outside preview pages it restores the selected Note-sequence snapshot; accidental `GRID 8` immediate `Init Steps` can therefore be reverted safely
- Align `Chaos` `ResetGen` with the other generators by keeping the current seed and current scope (`Vandalize` or `Wreck`) while resetting Chaos parameters
- Make Euclidean encoder rotation reroll (`NEW EUCL`) when no `F3/F4/F5` parameter key is held, while keeping `F3/F4/F5 + encoder` as direct `Steps/Beats/Offset` edits
- Harden `Init Layer` / `Init Steps` consistency across `Note`, `Stochastic`, `Arp`, `Curve`, and `Logic` step pages by centralizing the shared `selection-or-full-track` fallback used by generator init paths
- Add extra defensive context guards around `Acid` / `Chaos` generator entry from `Note` step edit flows, so those actions are ignored if the active track is no longer a `Note` track
- Harden Launchpad `Generators Mode` scene-key handling so track retargeting stays locked while the machine is inside the `Acid` / `Chaos` selector path
- Harden modal dispatch so if a selector/modal closes during key handling, the same event is no longer redispatched to the underlying page
- Add regression coverage in the simulator test harness for generator footer stability and selector cancel leak paths, and make existing project-page tests baseline-independent from simulator demo defaults

# v0.3.2-vinx.1.5.1
- Restore the `LP Note Style` machine-setting default to `Circuit`, so the firmware matches the current Launchpad documentation and intended fork workflow again
- Fix generator reset semantics so `Init Layer` on `Steps` remains layer-only, `Init Steps` in the `GEN` chooser is a real all-step-layers reset, and both now use the current persistent selection first or the whole current track when no selection exists
- Fix `Init Layer` / `Init Steps` behavior on `Stochastic` and `Arp`: `Init Layer` no longer clears full steps, `Arp` `Note` layer initialization restores its expected chromatic default, and `Init Steps` preserves the expected chromatic defaults for the first 12 note slots instead of flattening them through plain step clears
- Fix `Stochastic Init Seq` so sequence clear restores the intended low/high octave defaults deterministically again
- Fix generator footer navigation so switching between footer options in `Random`, `Acid`, and `Chaos` no longer exits the page unexpectedly
- Lock LP track switching while the machine is inside the `Acid` / `Chaos` selector path, so those modal choices stay bound to the current track until you cancel or confirm them on the machine
- Make encoder press commit the generator directly in `Random`, `Acid`, and `Euclidean`, so those pages can apply without reaching for `F5`

# v0.3.2-vinx.1.5.0 (2 April 2026)
- Refine the Chaos generator pages: rename the labels from CHAOS ON SEQ / CHAOS ON PAT to VNDLZ SEQ / WRECK PAT, and show the active seed in both Vandalize Sequence and Wreck Pattern while the untouched source still reads ORIGINAL
- Fix LP Note Style: Classic drawing on the Launchpad Note layer so non-home navigation positions light the edited note correctly instead of only updating the underlying step value
- Fix Stochastic -> Note Prob and Arp -> Note Launchpad handling so the dedicated Circuit keyboard/editor is used only when LP Note Style = Circuit, while Classic returns to the normal layer/navigation view
- Change the default LP Note Style machine setting from Classic to Circuit
- Make Launchpad input wake the display from the screensaver like panel input
- Fix Stochastic Circuit Keyboard probability and double-press targeting so 1..16 and note-toggle actions follow the actual selected note slot instead of drifting through stale octave-aware selection state
- Remove the non-editable derived Rest Prob display from the first Launchpad Rest Probability row, leaving only the editable Rest Prob 2 / 4 / 8 ranges
- Fix Launchpad behavior in Pattern, Performer, and Note Circuit Keyboard: edited Arp patterns are shown again in Pattern, Performer TOP 7 + TRK fills are released correctly, and Note Circuit Keyboard step editing now matches the documented 1..16 range
- Add an experimental Launchpad Generators Mode for Note tracks in LP Sequence mode: TOP 8 + TOP 4 jumps to Steps when needed and toggles the mode, GRID 1 / 2 / 10 / 3 / 11 / 4 / 8 select Random, Acid Phrase, Acid Layer, Vandalize, Wreck, Euclidean, and Init Steps, pressing the currently selected generator pad rerolls it in place, and TOP 5 / 6 / 7 / 8 map to A/B, ResetGen, Cancel, and Apply
- Refine Launchpad Generators Mode feedback and routing so TOP 5 distinguishes A / B, TOP 6 distinguishes reset-neutral from generated/edited state, unmapped GRID pads and TOP 1..3 stay neutral, TRK 1..8 can retarget the mode to another Note track, and off-context controls no longer trap the user until Cancel, Apply, or the mode toggle
- Fix remaining modal selector regressions introduced by modal redispatch so Confirmation, File Select, and Chaos Defaults no longer leak key presses to the underlying page when they close or ignore non-context keys
- Remove stray #include <iostream> usage from the STM32 sequencer code so the firmware no longer drags in the full iostream / locale runtime, recovering a large amount of flash and SRAM
- Clean up minor simulator build warnings by replacing boolean bitwise | usage in ListPage / QuickEditPage and removing an unused ArpTrackEngine field
- Replace the VersionedSerializedReader variable-length skip buffer with fixed-size chunked reads, removing the remaining Clang VLA warning in simulator builds
- Rework the Launchpad cheatsheet/doc site into a split Reference + tables format and update the manual/cheatsheet wording around LP Note Style, Stochastic Circuit Keyboard, Arp Circuit Keyboard, and Launchpad-specific workflows

# v0.3.2-vinx.1.4.9 (31 March 2026)
- Refine external clock input behavior by adding `Reset Pulse` as a new mode alongside `Reset Gate`, `Run`, and `Start/Stop`, and make it respond only to the rising edge so reset pulses resync the slave clock without turning the falling edge into an implicit start
- Add per-step `Gate Offset` and `Gate Length` layers to `Curve` tracks, including edit-page, overview, and Python bindings support
- Fix `Curve`-track entry and Launchpad handling when the new `Gate Offset` or `Gate Length` layers are selected
- Restore `PAGE + S7` undo on `Curve` step editing so it matches the existing Note-track edit workflow
- Rework machine-settings save flow: add direct `F2 = SAVE` on `System` for both `Calibration` and `Settings`, simplify the context menu to `Init`, `Backup`, and `Restore`, return `Chaos Defaults` to the normal save path, and prompt to save when leaving `System -> Settings` or leaving the `System` page after user-setting changes while keeping `Calibration` outside that check
- Add `Menu Wrap` as an on-by-default machine preference for cyclic list navigation
- Fix scale selector limits across `Project`, `Note`, `Logic`, `Stochastic`, and `Arp`
- Make MIDI note capture respect the active scale octave span in `Note`, `Stochastic`, and `Arp`
- Change the Launchpad defaults to `LP Style = Blue` and `LP Note Style = Classic`
- Turn the Desktop Simulator into a more usable MIDI/Launchpad tool: make DIN and USB MIDI ports configurable from the command line, start with MIDI disabled by default, add DIN/USB status indicators plus `--trace-midi` and `--trace-dio`, infer the connected Launchpad model from USB port names, and mirror sibling `DAW` / `MIDI` ports so Launchpad pad presses and initialization messages still reach the correct endpoint
- Current validation scope: real hardware testing is still useful for `Reset Pulse` / `Reset Gate` and for `Voltage Mode` user scales in `Note`, `Arp`, and `Stochastic`; Desktop Simulator USB MIDI is validated well on `macOS / OS X` with `Launchpad Mini MK3`, but not yet across other Launchpads or operating systems

# v0.3.2-vinx.1.4.8
- Extend `Chaos` with an experimental `CHAOS MODE` split between `Vandalize Sequence` and `Wreck Pattern`, with `Wreck Pattern` applying a multi-layer, pattern-wide workflow across all Note tracks in the current pattern
- Refine the `Chaos` workflow so both modes enter on `ORIGINAL`, require an explicit `CHAOS` press, support `A/B` preview compare, show dedicated `VANDALIZED` / `WRECKED` popups, and use per-track `First Step` / `Last Step` ranges when `Wreck Pattern` runs without an explicit selection
- Add `System -> Chaos Defaults` with separate `Seq Layers to Vandalize` and `Pat Layers to Wreck`, edited through the same 4x4 grid and auto-saved on page close
- Refine the wider generator workflow: add stronger contextual labels (`ACID PHRASE`, `ACID LAYER`, `CHAOS ON SEQ`, `CHAOS ON PAT`), fit the generator selector on one screen, rework `Euclidean` around `A/B`, `NEW EUCL`, and direct `Steps` / `Beats` / `Offset` editing with `Beats <= Steps` enforced, and make `RESETGEN` restore neutral defaults instead of rerolling
- Improve stability, memory use, and responsiveness around generator handling: keep transport (`PLAY/STOP`, `TEMPO`) active during generator use, default `System` to `Settings`, resolve generator re-entry and stale-state issues, limit `Vandalize Sequence` storage to the active Note sequence, and reduce `Wreck Pattern` memory usage by storing only target-step backups and regenerating previews in place
- Add a warning popup before entering `Chaos -> Wreck Pattern` (`F1` = `PROJ PAGE`, `F3` = `WRECK`, `F5` = cancel) and keep it explicitly marked as a wildly experimental pattern-wide process that should be used only after saving the project

# v0.3.2-vinx.1.4.7 (25 March 2026)
- Add `Chaos` as a new experimental Note-track generator, extending the generator order to `Random`, `Acid`, `Chaos`, `Euclidean`, `Init`, and define it as a non-destructive multi-layer macro-random tool driven by `Amount`, `A/B`, `Cancel`, `Apply`, and a new on-demand 32-bit seed
- Give `Chaos` its own 4x4 layer matrix with direct `All On` / `All Off` controls, and make it selection-aware like the other generators by targeting the persistent selection first and falling back to the current `First Step` / `Last Step` window
- Add Chaos-specific compare messaging so `A/B` flips between `ORIGINAL` and `VANDALIZED` while keeping the standard preview/apply safety model intact
- Refine Note-like step visualization again by moving retrigger marks into the step box, adding a compact condition mark next to the step number, and rendering slide as a small tie between adjacent step boxes
- Refine defaults and workflow around the wider line: set the screensaver default to `15m`, add `Wake Mode = required`, restore the correct `Init Layer` behavior while clarifying `Init Layer` vs `Init Steps`, and refresh the built-in simulator state and browser audio mapping

# v0.3.2-vinx.1.4.6 (24 March 2026)
- Reduce UI/display overhead by lowering the default refresh rate to `30 fps` and skipping display redraws when the framebuffer hash has not changed, avoiding unnecessary display traffic without adding a second framebuffer
- Add immediate `Scale` prelisten preview on Note and Arp sequence pages, while keeping commit on encoder press and adding `CANCEL` to `Scale` and `Root Note` editing so preview changes can be discarded safely
- Refine defaults and cleanup around sequencing/generators: set `Slide Time` to `20%` by default where available, remove the duplicate `Euclidean` entry from non-Note generator menus, and clean dead `PatternPage` allocation code
- Recover RAM headroom and refresh the simulator state by disabling `Task Profiler` and the `Asteroids` easter egg in active builds, then updating the bundled simulator/browser demo setup with a revised Track 8 synth voice and drum sample set

# v0.3.2-vinx.1.4.5 (22 March 2026)
- Refine generator preview rendering on the display: make the 16-step bank indicator thinner and framed, slim down the playback playhead, improve `Random` preview rendering, and align `Acid` `Slide` / `Phrase` note and slide lanes more cleanly
- Make `Random` preview layer-aware on Note tracks, reusing `Acid`-style Gate / Note / Slide visuals where they fit, adding dedicated views for Length and repeat-style layers, and reorder the `F4` Note layer cycle to `Note`, `Slide`, `Note Range`, `Note Prob`, `Bypass Scale`

# v0.3.2-vinx.1.4.4 (22 March 2026)
- Redesign generator previews on the display: `Random` now uses a centered 64-step baseline graph, `Acid` gets dedicated Note / Gate / Slide / Phrase preview layouts, and generator pages keep the current 16-step bank visible within the full 64-step view with a playback-following playhead

# v0.3.2-vinx.1.4.3 (21 March 2026)
- Add `Acid` as a new Note-track generator with non-destructive preview and a `Layer / Phrase` split: `Acid -> Layer` targets the active `Gate`, `Note`, or `Slide` layer, while `Acid -> Phrase` writes a coordinated `Gate + Note + Slide` phrase over the current selection or pattern length
- Refine generator defaults and menus: make `Range` displays consistently percentage-based, randomize parameters on entry while keeping `Variation = 100%`, tighten `Acid` `Density` / `Slide` behavior around deterministic target counts, reorder the generator menu to `Random`, `Acid`, `Euclidean`, `Init`, keep `Random Bias = 0` on entry, and trim `Acid -> Layer` to layer-relevant parameters
- Rework `NEW RAND` behavior so it differs from encoder seed edits: in `Random` it refreshes `Seed`, `Smooth`, and `Range` while leaving `Bias` untouched; in `Acid -> Layer` it refreshes `Seed` plus the active layer’s main parameter without touching `Variation`, and the same action is mirrored on `F5` for quicker access

# v0.3.2-vinx.1.4.2 (20 March 2026)
- Expand `Dim Sequence` from a binary toggle to `off`, `dim`, and `dim+`, defaulting to `dim` to better tame display noise leaking into the audio band

# v0.3.2-vinx.1.4.1 (20 March 2026)
- Remove the small step markers between step numbers in Note and Logic step views
- Restore Random `Variation` to `100%` by default

# v0.3.2-vinx.1.4 (19 March 2026)
- Rework `Generate -> Random` around full 32-bit hexadecimal seeds, automatic seed refresh on entry and seed edits, cleaner `NEW SEED` / `INIT` / `CANCEL` / `APPLY` flow, and `F1 = A/B` compare between `ORIGINAL` and `CURRENT SEED`
- Refine Random generator behavior and preview stability: keep `Seed` stable while editing other parameters, make `Variation` update the preview on the display correctly, and set the default `Variation` to `50%` with clearer A/B seed-slot feedback
- Improve generator/page navigation by opening `Generate` on the current 16-step bank and making its LEDs and `prev` / `next` bank navigation follow that bank
- Refine defaults and visuals across the sequencer: set `Reset CV` to `Off`, return `Dim Sequence` to `Off`, improve step visualization for gate offset / length / retrigger, and further refine retrigger alignment in step rendering

# v0.3.2-vinx.1.3 (18 March 2026)
- Rework `Generate -> Random` around a non-destructive preview workflow with `A/B`, `Regenerate`, `Cancel`, `Apply`, and the new `Variation` parameter for controlled deviation from the original sequence
- Reorder the generator menu to `Random`, `Euclidean`, `Init`, and make `prev` / `next` sequence navigation wrap according to the current sequence length

# v0.3.2-vinx.1.2 (17 March 2026)
- With "Dim Sequence" enabled, step LEDs are slightly dimmed, which may help reduce electrical noise and interference. This option is now enabled by default.

# v0.3.2-vinx.1.1 (17 March 2026)
- Change the default output clock pulse from 1ms to 10ms for broader module compatibility
- Rescale the Swing UI from 50%-75% to 0%-99% while keeping the engine timing unchanged at 50%-75%, so older projects retain the same playback behavior

# v0.3.2-vinx.1 (16 March 2026)
- Fix step-shifting stability and range handling across note, logic, curve, stochastic, and arp sequences
- This includes the 64-step crash/reboot case and selected-step shifts inside non-zero subranges

## Mebitek fork history

# v0.3.2 ()
- issue #123 - request - launchpad X step page responsive

# v0.3.1 (29 May 2024)
- issue #111 - fix program change
- issue #112 - fix launchpad follow mode
- issue #115 - follow pattern is now saved on project
- issue #116 - fix F1-F5 shortcut
- issue #117 - fix curve cv problem
- issue #118 - fix file stack overflow
- fix fill display
- fix stochastic reseed behaviour
- fix arp track copy pattern


# v0.3.0 (02 May 2024)
- Arpeggiator Track
- increase stochastic rest proability to 15 steps
- logic track fast input visulaization (press shift)
- fix clipboard copy track
- fix logic gate probability
- fix rotate steps [@glibersat](https://github.com/glibersat)
- keep sequence names when copy and paste
- issue #100 - reorganize quick functions shortcuts
- issue #101 - reset cv in stop parameter
- integrate Malekko integration

> **testers** :
>
> mebitek, Jil, Guillaume Libersat, XQSTKRPS, P.M. Lenneskog


# v0.2.2 (20 March 2024)
- Logic Track
    - per step gate logic operators
    - per step note logic operators
- fix shift steps feature [@glibersat](https://github.com/glibersat)
- step recorder "move step forward" shortcuts
- step recorder "current step" cv routable, respond to gate (5ms)
- curve track cv contrallable min and max
- Overview page improvements
- add trigger curve shape
- add filter note parameter

> **testers** :
>
> mebitek, Jil, Guillaume Libersat, Andreas Hieninger, P.M. Lenneskog

# v0.2.1 (4 March 2024)
- issue #80 - Repeat Function Issue - Metropolix Mode
- issue #82 - Fatal error when pressing STEP button and turning encoder
- issue #83 - Restart when loop is on 64 steps
- issue #88 - Copy Loop from STOCHASTIC channel to a NOTE channel
- issue #90 - the first note is never recorded
- fix user settings

> **testers** :
>
> mebitek, Jil, Andreas Hieninger

# v0.2.0 (29 Febrary 2024)
- Stochastic Track
    - global octave modifier
    - launchpad control general octave
    - loop and lock loop
    - reseed
    - rest probability 2,4,8 steps
    - global gate length modifier
    - clipboard actions
    - generators
- Load/Save Sequence to use a sequence library (fast switch on loading)
- Launchpad Performance Mode `8`+`3` (`2`+`GRID2` -> quick set lenght sequence; `2`+`GRID1` -> overview page )
- Submenu shortcuts (double click F[1-5] to enter project, layout, routing, midi out, user scale)
- Page buttons on launchpad circuit note edit
- Extend gate Lenght to 4bits [@glibersat](https://github.com/glibersat)
- Multi Curve CV Recording (cv curve input has been moved in track page)
- quick change octave shortuct (step+F[1-5]) 1-5V
- quick gate accent launchpad control on gate page and circuit page (`7`+`GRID`)
- add steps to stop feature in project page. Once started when the engine reaches the steps to stop value the clock will stop.
- improved overview page. quick edit tracks

> **testers** :
>
> mebitek, Guillaume Libersat, Nick Ansell, XponentOne, dblu2000, hales2488, XQSTKRPS, KittenVillage, Andreas Hieninger

# v0.1.48 (30 January 2024)
- Moving steps in a sequence
- INIT by step selected
- smart cycling on patter follow modes (check if launchpad is connected)
- Show launchpad settings only when a launchpad is connected
- Apply random for selected steps only
- double click page to enter context menu for 2 seconds
- Prevent very short output clock pulses at higher BPMs
- Undo function (alt+s7)
- Curve mode backward run modes play reverse playback
- Bypass the Voltage Table in specific steps of the sequence

# v0.1.47 (24 January 2024)
- launchpad circuit mode improvements
- random generator: random seed just on init method
- patter follow [@glibersat](https://github.com/glibersat)
- pattern chain quick shortcut from pattern page
- scale edit: scales are changed only if the encoder is pressed
- on scale change the sequence notes are changed according to the previous scale. if a note in the previous scale is also present in the new scale the value is preeserved. if it is not present the nearest note in the new scale will be selected
- launchpad follows pattern in song mode

## v0.1.46 (4 January 2024)

- UI note edit page reaggment [@aclangor](https://github.com/aclangor)
- song sync with clock
- fix reverse shape feature
- add new shapes
- add new repeat modes
- encoder to change tempo on Performer page

## v0.1.45 (31 December 2023)

#### Improvements

- various bugfixes
- improve solo perform
- launchpad follow mode
- launchpad improve slide visualization
- launchpad color theme
- launchpad circuit dtyle note editor

## v0.1.44 (29 December 2023)

#### Improvements

- add curve improvements [thanks to jackpf](https://github.com/jackpf/performer/blob/master/doc/improvements/shape-improvements.md)
- add midi improvements [thanks to jackpf](https://github.com/jackpf/performer/blob/master/doc/improvements/midi-improvements.md)
- add noise reduction [thanks to jackpf](https://github.com/jackpf/performer/blob/master/doc/improvements/noise-reduction.md)
- add track names
- add quick tie notes feature

## v0.1.43 (27 December 2023)

#### Improvements

- Extend retrigger value. max 8 retriggers per step
- Extend probability steps. min value now is 6.3%
- add negative gate offset feature [thanks to vafu](https://github.com/vafu/performer/tree/vafu/negative-offset)
- add metropolis style sequencer option [thanks to vafu](https://github.com/vafu/performer/tree/vafu/metro-pr)
- double click to toggle gates when editing layers other than gate
- do not reset cv outputs when clock is stopped
- add various curves
- use random seed each access to random gen layer

## v0.1.42 (6 June 2022)

#### Fixes

- Support Launchpad Mk3 with latest firmware update (thanks to Novation support)

## v0.1.41 (18 September 2021)

#### Improvements

- Add support for Launchpad Pro Mk3 (thanks to @jmsole)
- Allow MIDI/CV track to only output velocity (#299)

#### Fixes

- Fix quick edit (#296)
- Fix USB MIDI (thanks to @av500)

## v0.1.40 (9 February 2021)

Note: This release changes the behavior of how note slides are generated. You may need to adjust the _Slide Time_ on existing projects to get a similar feel.

#### Improvements

- Improved behavior of slide voltage output (#243)
- Added _Play Toggle_ and _Record Toggle_ routing targets (#273)
- Retain generator parameters between invocations (#255)
- Allow generator parameters to be initialized to defaults
- Select correct slot when saving an autoloaded project (#266)
- React faster to note events when using MIDI learn
- Filter NPRN messages when using MIDI learn

#### Fixes

- Fixed curve playback cursor when track is muted (#261)
- Fix null pointer dereference in simulator (#284)

## v0.1.39 (26 October 2020)

#### Improvements

- Added support for Launchpad Mini MK3 and Launchpad X (#145)
- Improved performance of sending MIDI over USB

## v0.1.38 (23 October 2020)

#### Fixes

- Fixed MIDI output from monitoring during playback (#216)
- Fixed hanging step monitoring when leaving note sequence edit page

#### Improvements

- Added _MIDI Input_ option to select MIDI input for monitoring/recording (#197)
- Added _Monitor Mode_ option to set MIDI monitoring behavior
- Added double tap to toggle gates on Launchpad controller (#232)

## v0.1.37 (15 October 2020)

#### Fixes

- Output curve track CV when recording (#189, #218)
- Fix duplicate function on note/curve sequence page (#238)
- Jump to first row when switching between user scales
- Fixed printing of route min/max values for certain targets

#### Improvements

- Added _Fill Muted_ option to note tracks (#161)
- Added _Offset_ parameter to curve tracks (#221)
- Allow setting swing on tempo page when holding `PERF` button
- Added inverted loop conditions (#162)
- Improved step shifting to only apply in first/last step range (#196)
- Added 5ms delay to CV/Gate conversion to avoid capturing previous note (#194)
- Allow programming slides/ties using pitch/modulation control when step recording (#228)
- Added _Init Layer_ generator that resets the current layer to its default value (#230)
- Allow holding `SHIFT` for fast editing of route min/max values

## v0.1.36 (29 April 2020)

#### Fixes

- Update routings right after updating each track to allow its CV output to accurately modulate the following tracks (#167)

#### Improvements

- Added fill and mute functions to pattern mode on Launchpad (#173)
- Added mutes to song slots (#178)
- Added step monitoring on curve sequences (#186)
- Added a `hwconfig` to support DAC8568A (in addition to the default DAC8568C)

## v0.1.35 (20 Jan 2020)

#### Fixes

- Fix loading projects from before version 0.1.34 (#168)

## v0.1.34 (19 Jan 2020)

**PLEASE DO NOT USE THIS VERSION, IT CONTAINS A BUG PREVENTING IT FROM READING OLD PROJECT FILES!**

#### Fixes

- Fix inactive sequence when switching track mode (#131)

#### Improvements

- Added _Scale_ and _Root Note_ as routing targets (#166)
- Expanded number of MIDI outputs to 16 (#159)
- Expanded routable tempo range to 1..1000 (#158)
- Generate MIDI output from track monitoring (#148)
- Allow MIDI/CV tracks to consume MIDI events (#155)
- Default MIDI output note event settings with velocity 100
- Indicate active gates of a curve sequence on LEDs

## v0.1.33 (12 Nov 2019)

#### Fixes

- Fixed handling of root note and transposing of note sequences (#147)

#### Improvements

- Add mute mode to curve tracks to allow defining the mute voltage state (#151)
- Increased double tap time by 50% (#144)

## v0.1.32 (9 Oct 2019)

#### Fixes

- Fix _Latch_ and _Sync_ modes on permanent _Performer_ page (#139)

## v0.1.31 (15 Aug 2019)

#### Improvements

- Added _Slide Time_ parameter to MIDI/CV track (#121)
- Added _Transpose_ parameter to MIDI/CV track (#128)
- Allow MIDI/CV tracks to be muted

#### Fixes

- Use _Last Note_ note priority as default value on MIDI/CV track
- Added swing to arpeggiator on MIDI/CV track

## v0.1.30 (11 Aug 2019)

This release is mostly dedicated to improving song mode. Many things have changed, please consult the manual for learning how to use it.

#### Improvements

- Added _Time Signature_ on project page for setting the duration of a bar (#118)
- Increased the number of song slots to 64 (#118)
- Added context menu to song page containing the _Init_ function (#118)
- Added ability to duplicate song slots (#118)
- Show song slots in a table (#118)
- Allow song slots to play for up to 128 bars (#118)
- Improved octave playback for arpeggiator (#109)

#### Fixes

- Do not reset MIDI/CV tracks when switching slots during song playback
- Fixed some edge cases during song playback

## v0.1.29 (31 Jul 2019)

#### Improvements

- Added ability to select all steps with an equal value on the selected layer using `SHIFT` + double tap a step (#117)
- Added _Note Priority_ parameter to MIDI/CV track (#115)

## v0.1.28 (28 Jul 2019)

#### Improvements

- Allow editing first/last step parameter together (#114)

#### Fixes

- Fixed rare edge case where first/last step parameter could end up in invalid state and crash the firmware
- Fixed glitchy encoder by using a full state machine decoding the encoders gray code (#112)

## v0.1.27 (24 Jul 2019)

This release primarily focuses on improvements of curve tracks.

#### Improvements

- Added _shape variation_ and _shape variation probability_ layers to curve sequences (#108)
- Added _gate_ and _gate probability_ layer to curve sequences (#108)
- Added _shape probability bias_ and _gate probability bias_ parameters to curve tracks (#108)
- Added better support for track linking on curve tracks including recording (#108)
- Added specific fill modes for curve tracks (variation, next pattern and invert) (#108)
- Added support for mutes on curve tracks (keep CV value at last position and mute gates) (#108)
- Added support for offsetting curves up and down by holding min or max function buttons (#103)

## v0.1.26 (20 Jul 2019)

#### Improvements

- Evaluate transposition (octave/transpose) when monitoring and recording notes (#102)
- Added routing target for tap tempo (#100)

## v0.1.25 (18 Jul 2019)

#### Improvements

- Allow chromatic note values to be changed in octaves by holding `SHIFT` (#101)
- Swapped high and low curve types and default curve sequence to be all low curves (#101)
- Show active step on all layers during step recording

#### Fixes

- Fixed euclidean generator from being destructive (#97)
- Fixed handling of active routing targets (#98)
- Fixed some edge cases in the step selection UI
- Fixed ghost live recording from past events
- Fixed condition layer value editing (clamp to valid values)

## v0.1.24 (9 Jul 2019)

#### Fixes

- Fixed writing incorrect project version

## v0.1.23 (8 Jul 2019)

**PLEASE DO NOT USE THIS VERSION, IT WRITES THE INCORRECT PROJECT FILE VERSION!**

#### Improvements

- Check and disallow to create conflicting routes (#95)
- Added new _Note Range_ MIDI event type for routes (#96)

#### Fixes

- Fixed track selection for new routing targets added in previous release
- Fixed hidden editing of route tracks

## v0.1.22 (6 Jul 2019)

#### Improvements

- Pressing `SHIFT` + `F[1-5]` on the sequence edit pages selects the first layer type of the group (#84)
- Solo a track is now done with `SHIFT` + `T[1-8]` on the performer page (#87)
- Added ability to hold fills by pressing `SHIFT` + `S[9-16]` on the performer page (#65)
- Added new _Fill Amount_ parameter to control the amount of fill in effect (#65)
- Added new routing targets: Mute, Fill, Fill Amount and Pattern (#89)
- Added new _Condition_ layer to note sequences to conditionally trigger steps (#13)
- Do not show divisor denominator if it is 1

#### Fixes

- Fixed unreliable live recording of note sequences (#63)
- Fixed latching mode when selecting a new global pattern on pattern page

## v0.1.21 (26 Jun 2019)

#### Improvements

- Added recording curve tracks from a CV source (#72)
- Added ability to output analog clock with swing (#5)
- Added sequence progress info to performer page (#79)

#### Fixes

- Fixed navigation grid on launchpad when note/slide layer is selected

## v0.1.20 (15 Jun 2019)

This is mostly a bug fix release. From this release onwards, binaries are only available in intel hex format, which is probably what most people used anyway.

#### Improvements

- Allow larger range of divisor values to enable even slower sequences (#61)
- Added hwconfig option for inverting LEDs (#74)

#### Fixes

- Fixed changing multiple track modes at once on layout page
- Fixed MIDI channel info on monitor page (show channel as 1-16 instead of 0-15)
- Fixed "hidden commit button" on layout page (#68)
- Fixed race condition when initializing a project
- Fixed detecting button up events on launchpad devices

## v0.1.19 (18 May 2019)

#### Improvements

- Added arpeggiator to MIDI/CV track (#47)
- Show note names for chromatic scales (#55)
- Added divisor as a routable target (#53)
- Improved free play mode to react smoothly on divisor changes (#53)

#### Fixes

- Handle _alternative_ note off MIDI events (e.g. note on events with velocity 0) (#56)
- Update linked tracks immediately not only when track mode is changed

## v0.1.18 (8 May 2019)

#### Improvements

- Added _Slide Time_ parameter to curve tracks to allow for slew limiting (#40)

#### Fixes

- Fixed delayed CV output signals, now CV signal is guaranteed to be updated **before** gate signal (#36)

## v0.1.17 (7 May 2019)

**Note: The new features added by this release are not yet heavily tested. You can always downgrade should you experience problems, but please report them.**

#### Improvements

- Added gate offset to note sequences, allowing to delay gates (#14)
- Added watchdog supervision (#31)

## v0.1.16 (5 May 2019)

#### Improvements

- Improved UI for editing MIDI port/channel (#39)

#### Fixes

- Fixed tied notes when output via MIDI (#38)
- Fixed regression updating gate outputs in tester application (#37)

## v0.1.15 (20 Apr 2019)

#### Improvements

- Added CV/Gate to MIDI conversion to allow using a CV/Gate source for recording (#32)

#### Fixes

- Fixed setting gate outputs before CV outputs (#36)

## v0.1.14 (17 Apr 2019)

#### Improvements

- Removed switching launchpad brightness in favor of full brightness
- Enhanced launchpad pattern page
    - Show patterns that are not empty with dimmed colors (note patterns in dim yellow, curve patterns in dim red) (#25, #26)
    - Show requested patterns (latched or synched) with dim green

#### Fixes

- Fixed saving/loading user scales as part of the project
- Fixed setting name when loading user scales (#34)

## v0.1.13 (15 Apr 2019)

#### Fixes

- Fixed Launchpad Pro support
- Fixed _Reset Measure_ when _Play Mode_ is set to _Aligned_ (#33)

## v0.1.12 (7 Apr 2019)

#### Improvements

- Refactored routing system to be non-destructive (this means that parameters will jump back to the original value when a route is deleted) (#27)
- _First Step_, _Last Step_ and _Run Mode_ routes affect every pattern on a track (instead of just the first one)
- Indicate routed parameters in the UI with an right facing arrow
- Added `ROUTE` item in context menu of sequence pages to allow creating routes (as in other pages)
- Added support for Launchpad Pro

## v0.1.11 (3 Apr 2019)

This release is mostly adding support for additional launchpad devices. However, while testing I unfortunately found that the hardware might have an issue on the USB power supply (in fact there is quite some ripple on the internal 5V power rail) leading to spurious resets of the launchpad. Connecting the launchpad through an externally powered USB hub seems to fix the issue. Therefore it is beneficial to run the launchpad with dimmed leds (default). For now, the brightness can be adjusted by pressing `8` + `6` on the launchpad. I will investigate the hardware issue as soon as possible.

#### Improvements

- Added preliminary support for Launchpad S and Launchpad Mk2

#### Fixes

- Fixed launchpad display regression (some sequence data was displayed in wrong colors)
- Fixed fill regression (fills did not work if track was muted anymore)

## v0.1.10 (1 Apr 2019)

#### Improvements

- Implemented _Step Record_ mode (#23)

## v0.1.9 (29 Mar 2019)

#### Improvements

- Improved editing curve min/max value on Launchpad (#19)
- Show octave base notes on Launchpad when editing notes (#18)
- Allow selecting sequence run mode on Launchpad

#### Fixes

- Fixed stuck notes on MIDI output

## v0.1.8 (27 Mar 2019)

#### Improvements

- Added `CV Update Mode` to note track settings to select between updating CV only on gates or always
- Added ability to use curve tracks as sources for MIDI output

#### Fixes

- Fixed copying curve tracks to clipboard (resulted in crash)

## v0.1.7 (23 Mar 2019)

#### Fixes

- Fixed glitchy encoder (#4)
- Implemented legato notes and slides via MIDI output (#3)
- Fixed routing and MIDI output page when clearing or loading a project

## v0.1.6 (17 Mar 2019)

#### Fixes

- Fixed MIDI output (note and velocity constant values were off by one before)

## v0.1.5 (1 Feb 2019)

### !!! This version is not backward compatible with previous project files !!!

#### Improvements

- `Run Mode` of sequences is now a routing target
- Added retrigger and note probability bias to track parameters and routing targets
- All probability values are now in the range of 12.5% to 100%
- Nicer visualization of target tracks in routing page
- Allow clock output pulse length of up to 20ms

## v0.1.4 (24 Jan 2019)

#### Improvements

- Rearranged quick edit of sequence parameters due to changes in frontpanel
- Changed the preset scales
- Note values are now displayed as note index and octave instead of note value and octave
- Flipped encoder reverse mode in hardware config to make default encoder in BOM act correctly
- Flipped encoder direction in bootloader

#### Bugfixes

- Fixed clock reset output state when device is powered on
- Fixed tap tempo
- Fixed pattern page when mutes are activated
- Fixed clock input/output divisors

## v0.1.3 (14 Nov 2018)

#### New Features

- Added MIDI output module
    - Generate Note On/Off events from gate, pitch and velocity of different tracks (or constant values)
    - Generate CC events from track CV
- Overview page
    - Press `PAGE` + `LEFT` to open Overview page
- Simple startup screen that automatically loads the last used project on power on
- Added new routing targets for controlling Play and Record

#### Improvements

- Sequence Edit page is now called Steps page
- Show track mode on Sequence page (same as on Track page)
- Improved route ranges
    - Allow +/- 60 semitone transposition
- Restyled pattern page
    - Differentiate from Performer page
    - Show selected pattern per track
- Exit editing mode when committing a Route or MIDI Output

#### Bugfixes

- Fixed exponential curves (do not jump to high value at the end)
- Fixed loading project name
- Fixed showing pages on leds
- Fixed launching patterns in latched mode
- Fixed setting edit pattern when changing patterns on Launchpad

## v0.1.2 (5 Nov 2018)

- No notes

## v0.1.1 (2 Oct 2018)

- Initial release
