import testframework as tf


class GeneratorRegressionTest(tf.UiTest):
    def _open_generator_select_from_steps(self):
        c = self.controller

        c.selectPage("steps")
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)

        # Open context menu (PAGE+SHIFT) and trigger GEN (F5) while modifiers are held.
        c.down("page").wait(10)
        c.down("shift").wait(10)
        c.press("f5").wait(30)
        c.up("shift").wait(10)
        c.up("page").wait(20)

        self.assertTrue(self.env.sequencer.isModalPageTop)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)

    def _open_generator_page(self, generator_index):
        c = self.controller

        self._open_generator_select_from_steps()

        for _ in range(generator_index):
            c.right().wait(10)

        # Confirm generator in selector.
        c.encoder().wait(30)

        # Acid and Chaos go through an extra modal selector before Generator page.
        if generator_index in (1, 2):
            self.assertTrue(self.env.sequencer.isModalPageTop)
            c.encoder().wait(30)

        self.assertTrue(self.env.sequencer.isGeneratorPageTop)

    def _lp_connect(self):
        seq = self.env.sequencer
        if not seq.launchpadControllerConnectedForTest:
            seq.connectLaunchpadForTest()
            self.controller.wait(50)
        self.assertTrue(seq.launchpadControllerConnectedForTest)

    def _lp_function_down(self, function_index):
        self.controller.midi(1, tf.core.MidiMessage.makeControlChange(0, 104 + function_index, 127)).wait(10)

    def _lp_function_up(self, function_index):
        self.controller.midi(1, tf.core.MidiMessage.makeControlChange(0, 104 + function_index, 0)).wait(10)

    def _lp_press_function(self, function_index):
        self._lp_function_down(function_index)
        self._lp_function_up(function_index)

    def _lp_press_grid(self, grid_index):
        row = grid_index // 8
        col = grid_index % 8
        # Launchpad Mk2 grid mapping used by test hook default device (product 0x0069).
        note = 11 + 10 * (7 - row) + col
        self.controller.midi(1, tf.core.MidiMessage.makeNoteOn(0, note, 127)).wait(10)
        self.controller.midi(1, tf.core.MidiMessage.makeNoteOff(0, note, 0)).wait(10)

    def _lp_press_scene(self, scene_index):
        # Launchpad Mk2 scene mapping.
        note = 11 + 10 * (7 - scene_index) + 8
        self.controller.midi(1, tf.core.MidiMessage.makeNoteOn(0, note, 127)).wait(10)
        self.controller.midi(1, tf.core.MidiMessage.makeNoteOff(0, note, 0)).wait(10)

    def _lp_toggle_generators_mode(self):
        # LP TOP 8 + TOP 4 toggle: hold Shift (TOP 8), press Function 4 (index 3), release Shift.
        self._lp_function_down(7)
        self._lp_press_function(3)
        self._lp_function_up(7)
        self.controller.wait(30)

    def _set_single_persistent_selection(self, step_button):
        c = self.controller
        c.down("shift").wait(10)
        c.press(step_button).wait(10)
        c.up("shift").wait(10)

    def _note_signature(self, sequence, count=16):
        return tuple(
            (sequence.steps[i].gate, sequence.steps[i].length, sequence.steps[i].note)
            for i in range(count)
        )

    def _curve_signature(self, sequence, count=16):
        return tuple(
            (sequence.steps[i].shape, sequence.steps[i].gate, sequence.steps[i].min, sequence.steps[i].max)
            for i in range(count)
        )

    def _set_generator_param_percent(self, function_button, value):
        c = self.controller
        c.down(function_button).wait(10)
        for _ in range(140):
            c.left()
        for _ in range(value):
            c.right()
        c.up(function_button).wait(10)

    def _assert_scene_switch_locked_in_generator_page(self, generator_index):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        p.setTrackMode(1, p.tracks[1].TrackMode.Curve)
        c.selectPage("steps")
        self._lp_connect()

        self._open_generator_page(generator_index)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        self._lp_press_scene(1)
        self.assertEqual(p.selectedTrackIndex, 0)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        c.press("f4").wait(20)  # Cancel

    def test_generator_footer_navigation_does_not_exit_page(self):
        c = self.controller

        # Random
        self._open_generator_page(0)
        c.press("f1").wait(20)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        c.press("f2").wait(20)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        c.encoder().wait(20)    # Apply (encoder commit on Random)
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)

        # Acid
        self._open_generator_page(1)
        c.press("f1").wait(20)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        c.press("f2").wait(20)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        c.encoder().wait(20)    # Apply (encoder commit on Acid)
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)

        # Euclidean
        self._open_generator_page(3)
        c.press("f1").wait(20)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        c.press("f2").wait(20)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        c.encoder().wait(20)    # Apply (encoder commit on Euclidean)
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)

        # Chaos
        self._open_generator_page(2)
        c.press("f1").wait(20)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        c.press("f3").wait(20)  # CHAOS action
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        c.press("f4").wait(20)  # Cancel
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)

    def test_seeded_generators_enter_on_original_and_generate_on_first_reroll(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        c.selectPage("steps")
        p.selectedNoteSequenceLayer = p.selectedNoteSequence.Layer.Note
        sequence = p.selectedNoteSequence

        def reset_note_baseline():
            for idx in range(16):
                step = sequence.steps[idx]
                step.gate = (idx % 2) == 0
                step.length = 6
                step.note = 24 + (idx % 8)
                step.slide = False

        def reset_gate_baseline():
            for idx in range(16):
                step = sequence.steps[idx]
                step.gate = False

        # Random: enter on ORIGINAL -> immediate apply is no-op.
        reset_note_baseline()
        before = self._note_signature(sequence, 16)
        self._open_generator_page(0)  # Random
        c.encoder().wait(30)          # Apply without reroll
        self.assertEqual(self._note_signature(sequence, 16), before)

        # Random: first reroll creates first preview and applies changes.
        reset_note_baseline()
        before = self._note_signature(sequence, 16)
        self._open_generator_page(0)
        c.right().wait(20)            # Encoder reroll (seed)
        c.encoder().wait(30)          # Apply
        self.assertNotEqual(self._note_signature(sequence, 16), before)

        # Acid: enter on ORIGINAL -> immediate apply is no-op.
        reset_note_baseline()
        before = self._note_signature(sequence, 16)
        p.selectedNoteSequenceLayer = p.selectedNoteSequence.Layer.Note
        self._open_generator_page(1)  # Acid
        c.encoder().wait(30)          # Apply without reroll
        self.assertEqual(self._note_signature(sequence, 16), before)

        # Acid: first reroll creates first preview and applies changes.
        reset_note_baseline()
        before = self._note_signature(sequence, 16)
        p.selectedNoteSequenceLayer = p.selectedNoteSequence.Layer.Note
        self._open_generator_page(1)
        c.right().wait(20)            # Encoder reroll (seed)
        c.encoder().wait(30)          # Apply
        self.assertNotEqual(self._note_signature(sequence, 16), before)

        # Euclidean: enter on ORIGINAL -> immediate apply is no-op.
        reset_gate_baseline()
        before = self._note_signature(sequence, 16)
        self._open_generator_page(3)  # Euclidean
        c.encoder().wait(30)          # Apply without reroll
        self.assertEqual(self._note_signature(sequence, 16), before)

        # Euclidean: first reroll creates first preview and applies changes.
        reset_gate_baseline()
        before = self._note_signature(sequence, 16)
        self._open_generator_page(3)
        c.right().wait(20)            # Encoder reroll (NEW EUCL path)
        c.encoder().wait(30)          # Apply
        self.assertNotEqual(self._note_signature(sequence, 16), before)

    def test_random_variation_uses_probabilistic_keep_original_semantics(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        c.selectPage("steps")
        p.selectedNoteSequenceLayer = p.selectedNoteSequence.Layer.Note

        sequence = p.selectedNoteSequence

        def reset_baseline():
            for idx in range(16):
                step = sequence.steps[idx]
                step.gate = True
                step.length = 6
                step.note = 30 + (idx % 8)
                step.slide = False

        def changed_steps(before, after):
            return sum(1 for idx in range(16) if before[idx] != after[idx])

        # 0% keeps original material.
        reset_baseline()
        before = self._note_signature(sequence, 16)
        self._open_generator_page(0)  # Random
        self._set_generator_param_percent("f2", 0)  # Var
        c.right().wait(20)  # first reroll
        c.encoder().wait(30)  # Apply
        after_zero = self._note_signature(sequence, 16)
        self.assertEqual(after_zero, before)

        # 100% applies generated material.
        reset_baseline()
        before = self._note_signature(sequence, 16)
        self._open_generator_page(0)  # Random
        self._set_generator_param_percent("f2", 100)  # Var
        c.right().wait(20)  # first reroll
        c.encoder().wait(30)  # Apply
        after_full = self._note_signature(sequence, 16)
        changed_full = changed_steps(before, after_full)
        self.assertGreater(changed_full, 0)

        # 50% is probabilistic: across multiple runs it should preserve more
        # original steps than a full 100% application.
        min_changed_half = 16
        for _ in range(6):
            reset_baseline()
            before = self._note_signature(sequence, 16)
            self._open_generator_page(0)  # Random
            self._set_generator_param_percent("f2", 50)  # Var
            c.right().wait(20)  # first reroll
            c.encoder().wait(30)  # Apply
            after_half = self._note_signature(sequence, 16)
            changed_half = changed_steps(before, after_half)
            min_changed_half = min(min_changed_half, changed_half)

        self.assertLess(min_changed_half, changed_full)

    def test_chaos_target_edit_invalidates_preview_and_apply_requires_explicit_chaos(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        c.selectPage("steps")
        p.selectedNoteSequenceLayer = p.selectedNoteSequence.Layer.Note
        sequence = p.selectedNoteSequence

        for idx in range(16):
            step = sequence.steps[idx]
            step.gate = True
            step.length = 6
            step.note = 24 + (idx % 8)
            step.slide = False

        before = self._note_signature(sequence, 16)
        self._open_generator_page(2)  # Chaos

        # Ensure we are actually on a generated preview before editing targets.
        preview = before
        for _ in range(4):
            c.press("f3").wait(30)  # CHAOS -> preview
            preview = self._note_signature(sequence, 16)
            if preview != before:
                break
        self.assertNotEqual(preview, before)

        c.down("shift").wait(10)
        c.press("step1").wait(10)  # selection edit -> invalidate preview
        c.up("shift").wait(10)
        # New behavior: edits invalidate APPLY eligibility but keep current preview visible.
        self.assertEqual(self._note_signature(sequence, 16), preview)

        c.press("f5").wait(20)  # Apply should be blocked until explicit CHAOS
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        self.assertEqual(self._note_signature(sequence, 16), preview)

        c.press("f3").wait(20)  # explicit CHAOS
        c.press("f5").wait(30)  # Apply
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)
        self.assertNotEqual(self._note_signature(sequence, 16), before)

    def test_chaos_preview_stays_visible_after_edit_and_cancel_restores_entry_state(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        c.selectPage("steps")
        p.selectedNoteSequenceLayer = p.selectedNoteSequence.Layer.Note
        sequence = p.selectedNoteSequence

        for idx in range(16):
            step = sequence.steps[idx]
            step.gate = True
            step.length = 8
            step.note = 30 + (idx % 6)
            step.slide = False

        before = self._note_signature(sequence, 16)
        self._open_generator_page(2)  # Chaos

        preview = before
        for _ in range(4):
            c.press("f3").wait(30)  # first explicit preview
            preview = self._note_signature(sequence, 16)
            if preview != before:
                break
        self.assertNotEqual(preview, before)

        c.down("shift").wait(10)
        c.press("step1").wait(10)  # selection edit while preview active
        c.up("shift").wait(10)
        self.assertEqual(self._note_signature(sequence, 16), preview)

        c.press("f4").wait(30)  # Cancel
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)
        self.assertEqual(self._note_signature(sequence, 16), before)

    def test_generator_and_selector_cancel_do_not_leak_to_underlying_page(self):
        c = self.controller
        p = self.env.sequencer.model.project

        # GeneratorSelect cancel should not trigger Note page function key handling.
        self._open_generator_select_from_steps()
        layer_before = p.selectedNoteSequenceLayer
        c.press("f4").wait(20)  # Cancel in GeneratorSelect modal
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)
        self.assertEqual(p.selectedNoteSequenceLayer, layer_before)

        # AcidModeSelect cancel should not leak to underlying Note page.
        self._open_generator_select_from_steps()
        c.right().wait(10)      # Acid
        c.encoder().wait(30)    # Open AcidModeSelect
        self.assertTrue(self.env.sequencer.isModalPageTop)
        layer_before = p.selectedNoteSequenceLayer
        c.press("f4").wait(20)  # Cancel in Acid selector
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)
        self.assertEqual(p.selectedNoteSequenceLayer, layer_before)

        # ChaosScopeSelect cancel should not leak to underlying Note page.
        self._open_generator_select_from_steps()
        c.right().right().wait(10)  # Chaos
        c.encoder().wait(30)        # Open ChaosScopeSelect
        self.assertTrue(self.env.sequencer.isModalPageTop)
        layer_before = p.selectedNoteSequenceLayer
        c.press("f4").wait(20)      # Cancel in Chaos selector
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)
        self.assertEqual(p.selectedNoteSequenceLayer, layer_before)

    def test_selector_confirm_is_fail_closed_if_track_changes_mid_modal(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        p.setTrackMode(1, p.tracks[1].TrackMode.Curve)

        # Random selector path: confirm must fail-close on mismatched context.
        self._open_generator_select_from_steps()
        p.selectedTrackIndex = 1
        c.encoder().wait(30)    # Confirm selector on mismatched context
        self.assertFalse(self.env.sequencer.isModalPageTop)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)

        # Restore note context for the Acid selector path.
        p.selectedTrackIndex = 0
        c.selectPage("steps")

        # Acid selector path: change selected track while selector is open, then confirm.
        self._open_generator_select_from_steps()
        c.right().wait(10)      # Acid
        c.encoder().wait(30)    # Open AcidModeSelect
        self.assertTrue(self.env.sequencer.isModalPageTop)
        p.selectedTrackIndex = 1
        c.encoder().wait(30)    # Confirm selector on mismatched context
        self.assertFalse(self.env.sequencer.isModalPageTop)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)

        # Restore note context for the Chaos selector path.
        p.selectedTrackIndex = 0
        c.selectPage("steps")

        # Chaos selector path: same fail-closed behavior.
        self._open_generator_select_from_steps()
        c.right().right().wait(10)  # Chaos
        c.encoder().wait(30)        # Open ChaosScopeSelect
        self.assertTrue(self.env.sequencer.isModalPageTop)
        p.selectedTrackIndex = 1
        c.encoder().wait(30)        # Confirm selector on mismatched context
        self.assertFalse(self.env.sequencer.isModalPageTop)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)

    def test_generator_commit_is_fail_closed_if_context_changes(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        p.setTrackMode(1, p.tracks[1].TrackMode.Curve)

        self._open_generator_page(0)  # Random
        p.selectedTrackIndex = 1
        c.encoder().wait(20)          # Apply should fail-close

        self.assertFalse(self.env.sequencer.isModalPageTop)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)

    def test_launchpad_scene_switch_is_locked_while_machine_selector_is_open(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        p.setTrackMode(1, p.tracks[1].TrackMode.Curve)

        self._lp_connect()
        c.selectPage("steps")

        # Generator selector level 1 opened from machine.
        self._open_generator_select_from_steps()
        self._lp_press_scene(1)  # Try switching to track 2 from LP
        self.assertEqual(p.selectedTrackIndex, 0)
        self.assertTrue(self.env.sequencer.isModalPageTop)
        c.press("f4").wait(20)   # Cancel selector

        # Acid selector opened from machine.
        self._open_generator_select_from_steps()
        c.right().wait(10)      # Acid
        c.encoder().wait(30)    # Open AcidModeSelect
        self.assertTrue(self.env.sequencer.isModalPageTop)

        self._lp_press_scene(1)  # Try switching to track 2 from LP
        self.assertEqual(p.selectedTrackIndex, 0)
        self.assertTrue(self.env.sequencer.isModalPageTop)
        c.press("f4").wait(20)   # Cancel selector

        # Chaos selector opened from machine.
        self._open_generator_select_from_steps()
        c.right().right().wait(10)  # Chaos
        c.encoder().wait(30)        # Open ChaosScopeSelect
        self.assertTrue(self.env.sequencer.isModalPageTop)

        self._lp_press_scene(1)  # Try switching to track 2 from LP
        self.assertEqual(p.selectedTrackIndex, 0)
        self.assertTrue(self.env.sequencer.isModalPageTop)
        c.press("f4").wait(20)   # Cancel selector

    def test_launchpad_scene_switch_is_locked_inside_random_page(self):
        self._assert_scene_switch_locked_in_generator_page(0)

    def test_launchpad_scene_switch_is_locked_inside_euclidean_page(self):
        self._assert_scene_switch_locked_in_generator_page(3)

    def test_launchpad_scene_switch_is_locked_inside_acid_page(self):
        self._assert_scene_switch_locked_in_generator_page(1)

    def test_launchpad_scene_switch_is_locked_inside_chaos_page(self):
        self._assert_scene_switch_locked_in_generator_page(2)

    def test_launchpad_track_select_enters_steps_from_project_like_pages(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        self._lp_connect()

        # Force Launchpad Sequence mode.
        self._lp_function_down(7)
        self._lp_press_function(0)
        self._lp_function_up(7)
        c.wait(30)

        pages = ("project", "layout", "routing", "midiout", "userscale", "clock")
        for page in pages:
            c.selectPage(page)
            self._lp_press_scene(0)  # TRK 1
            self.assertEqual(p.selectedTrackIndex, 0, f"{page}: selected track")
            self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop, f"{page}: jump to Steps")

    def test_launchpad_grid8_runs_init_layer_in_generators_mode(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        c.selectPage("steps")
        self._lp_connect()

        sequence = p.selectedNoteSequence
        for idx in range(4):
            sequence.steps[idx].gate = True
        p.selectedNoteSequenceLayer = sequence.Layer.Gate
        self._set_single_persistent_selection("step1")

        self._lp_toggle_generators_mode()

        # GRID 8 (index 7) = Init Layer (selection-aware), mode stays active.
        self._lp_press_grid(7)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)
        self.assertFalse(sequence.steps[0].gate)
        self.assertTrue(sequence.steps[1].gate)
        self.assertTrue(sequence.steps[2].gate)
        self.assertTrue(sequence.steps[3].gate)
        self.assertTrue(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

    def test_launchpad_non_note_generators_mode_entropy_subset(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Curve)
        c.selectPage("steps")
        self._lp_connect()

        sequence = p.tracks[0].curveTrack.sequences[0]
        before = self._curve_signature(sequence, 16)

        self._lp_toggle_generators_mode()
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)

        # GRID 2 is unmapped in non-Note subset.
        self._lp_press_grid(1)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)

        # GRID 3 = Entropy.
        self._lp_press_grid(2)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)

        # Reroll on selected generator pad (Entropy/CHAOS action), then apply.
        self._lp_press_grid(2)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        self._lp_press_function(7)
        c.wait(50)

        self.assertFalse(self.env.sequencer.isGeneratorPageTop)
        self.assertFalse(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

        after = self._curve_signature(sequence, 16)
        self.assertNotEqual(after, before)

    def test_launchpad_non_note_generators_mode_mapping_and_init(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Curve)
        c.selectPage("steps")
        self._lp_connect()

        sequence = p.tracks[0].curveTrack.sequences[0]
        sequence.steps[0].shape = 17
        sequence.steps[0].gate = 3
        sequence.steps[0].min = 22
        sequence.steps[0].max = 200

        # Random (GRID 1) opens generator page.
        self._lp_toggle_generators_mode()
        self._lp_press_grid(0)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        self._lp_press_function(6)  # TOP 7 Cancel
        c.wait(30)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)
        self.assertFalse(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

        # Euclidean (GRID 4) opens generator page.
        self._lp_toggle_generators_mode()
        self._lp_press_grid(3)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        self._lp_press_function(6)  # TOP 7 Cancel
        c.wait(30)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)
        self.assertFalse(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

        # Init Steps (GRID 16) applies immediately and exits mode.
        self._lp_toggle_generators_mode()
        self._lp_press_grid(15)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)
        self.assertFalse(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

        # Curve step defaults restored by Init Steps.
        self.assertEqual(sequence.steps[0].shape, 0)
        self.assertEqual(sequence.steps[0].gate, 0)
        self.assertEqual(sequence.steps[0].min, 0)
        self.assertEqual(sequence.steps[0].max, 255)

    def test_launchpad_non_note_generators_mode_switch_stays_active_on_stochastic_gate(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Stochastic)
        p.selectedStochasticSequenceLayer = p.selectedStochasticSequence.Layer.Gate
        c.selectPage("steps")
        self._lp_connect()

        self._lp_toggle_generators_mode()
        self.assertTrue(self.env.sequencer.launchpadGeneratorsModeActiveForTest)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)

        # Open Random first, then repeatedly switch among subset generators.
        self._lp_press_grid(0)  # Random
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        self.assertTrue(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

        for grid_index in (2, 3, 0, 2, 3, 0, 2, 3):
            self._lp_press_grid(grid_index)  # Entropy / Euclidean / Random
            self.assertTrue(self.env.sequencer.isGeneratorPageTop)
            self.assertTrue(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

        self._lp_press_function(6)  # TOP 7 Cancel
        c.wait(30)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)
        self.assertFalse(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

    def test_launchpad_init_steps_exits_generators_mode(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        c.selectPage("steps")
        self._lp_connect()

        self._lp_toggle_generators_mode()
        self.assertTrue(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

        # GRID 16 (index 15) = Init Steps, immediate apply + exit mode.
        self._lp_press_grid(15)
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)
        self.assertFalse(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

    def test_launchpad_toggle_exits_generators_mode_even_from_generator_preview(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        c.selectPage("steps")
        self._lp_connect()

        self._lp_toggle_generators_mode()
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)

        # Open Random from mini-mode.
        self._lp_press_grid(0)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)

        # Toggle must always close mini-mode and return to Steps.
        self._lp_toggle_generators_mode()
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)

    def test_machine_encoder_apply_exits_generators_mode_to_plain_steps(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        c.selectPage("steps")
        self._lp_connect()

        sequence = p.selectedNoteSequence
        p.selectedNoteSequenceLayer = sequence.Layer.Gate
        sequence.steps[4].gate = False

        self._lp_toggle_generators_mode()
        self._lp_press_grid(0)  # Random
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        self.assertTrue(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

        c.encoder().wait(30)  # Machine encoder apply
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)
        self.assertFalse(self.env.sequencer.isGeneratorPageTop)
        self.assertFalse(self.env.sequencer.launchpadGeneratorsModeActiveForTest)

    def test_top7_top8_undo_shortcut_matches_page_s7_on_note_steps(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        c.selectPage("steps")
        self._lp_connect()

        sequence = p.selectedNoteSequence
        p.selectedNoteSequenceLayer = sequence.Layer.Gate
        sequence.steps[0].gate = False

        # Refresh in-memory undo snapshot from current sequence.
        c.selectPage("project")
        c.selectPage("steps")

        sequence.steps[0].gate = True
        self.assertTrue(sequence.steps[0].gate)

        # Hold TOP 7, press TOP 8 => launchpad undo shortcut.
        self._lp_function_down(6)
        self._lp_press_function(7)
        self._lp_function_up(6)

        self.assertFalse(sequence.steps[0].gate)

        # Keep regression coverage focused on parity with PAGE+S7 shortcut semantics.

    def test_launchpad_acid_layer_falls_back_to_phrase_on_unsupported_layer(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        c.selectPage("steps")
        self._lp_connect()

        sequence = p.selectedNoteSequence
        p.selectedNoteSequenceLayer = sequence.Layer.Length  # unsupported for Acid Layer mode
        for i in range(16):
            sequence.steps[i].gate = False
            sequence.steps[i].length = 1

        self._lp_toggle_generators_mode()
        self._lp_press_grid(1)  # GRID 2 = Acid Layer
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)

        # Force Density to 100% (F3 + encoder in Acid Layer footer) so
        # fallback-to-Phrase is deterministic:
        # Phrase mode must create gates; unsupported Layer mode would keep all gates OFF.
        c.down("f3").wait(10)
        for _ in range(120):
            c.right().wait(2)
        c.up("f3").wait(20)

        # Entry starts on ORIGINAL; first reroll is required to build preview.
        self._lp_press_grid(1)
        self.assertTrue(any(sequence.steps[i].gate for i in range(16)))
        c.press("f4").wait(20)  # Cancel

    def test_launchpad_chaos_resetgen_keeps_wreck_scope(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.selectedPatternIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        p.setTrackMode(1, p.tracks[1].TrackMode.Note)
        c.selectPage("steps")
        self._lp_connect()

        seq0 = p.tracks[0].noteTrack.sequences[0]
        seq1 = p.tracks[1].noteTrack.sequences[0]

        for i in range(8):
            seq0.steps[i].gate = True
            seq0.steps[i].length = 7
            seq0.steps[i].note = 20 + i
            seq1.steps[i].gate = True
            seq1.steps[i].length = 9
            seq1.steps[i].note = 50 + i

        before_track1 = self._note_signature(seq1, 16)

        self._lp_toggle_generators_mode()

        # GRID 11 (index 10) = Wreck Pattern
        self._lp_press_grid(10)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)

        # TOP 6 (function index 5) = ResetGen, then reroll and apply.
        self._lp_press_function(5)
        self._lp_press_grid(10)
        self._lp_press_function(7)
        self.controller.wait(50)

        after_track1 = self._note_signature(seq1, 16)
        self.assertNotEqual(after_track1, before_track1)

    def test_euclidean_encoder_rotation_rerolls_pattern_without_function_keys(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)

        # Note mode generator list: Random, Acid, Chaos, Euclidean, Init Steps.
        self._open_generator_page(3)

        sequence = p.selectedNoteSequence
        initial = self._note_signature(sequence, 16)

        changed = False
        for _ in range(8):
            c.right().wait(20)
            current = self._note_signature(sequence, 16)
            if current != initial:
                changed = True
                break

        self.assertTrue(changed)
        c.press("f4").wait(20)
