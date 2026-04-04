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

    def _note_signature(self, sequence, count=16):
        return tuple(
            (sequence.steps[i].gate, sequence.steps[i].length, sequence.steps[i].note)
            for i in range(count)
        )

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

        # Chaos
        self._open_generator_page(2)
        c.press("f1").wait(20)
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        c.press("f3").wait(20)  # CHAOS action
        self.assertTrue(self.env.sequencer.isGeneratorPageTop)
        c.press("f4").wait(20)  # Cancel
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)

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

    def test_launchpad_grid16_undo_recovers_from_init_in_generators_mode(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        c.selectPage("steps")
        self._lp_connect()

        sequence = p.selectedNoteSequence
        sequence.steps[0].gate = True
        sequence.steps[0].length = 13
        sequence.steps[0].note = 46
        before = self._note_signature(sequence, 16)

        self._lp_toggle_generators_mode()

        # GRID 8 (index 7) = Init Steps
        self._lp_press_grid(7)
        after_init = self._note_signature(sequence, 16)
        self.assertNotEqual(after_init, before)

        # GRID 16 (index 15) = Undo
        self._lp_press_grid(15)
        after_undo = self._note_signature(sequence, 16)
        self.assertEqual(after_undo, before)

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

        # Force Density to 100% (F2 + encoder) so fallback-to-Phrase is deterministic:
        # Phrase mode must create gates; unsupported Layer mode would keep all gates OFF.
        c.down("f2").wait(10)
        for _ in range(120):
            c.right().wait(2)
        c.up("f2").wait(20)

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
