import testframework as tf


class InitRegressionTest(tf.UiTest):
    def _open_generator_select_from_steps(self):
        c = self.controller
        c.selectPage("steps")

        # Open context menu action GEN (PAGE+SHIFT+F5 while modifiers are held).
        c.down("page").wait(10)
        c.down("shift").wait(10)
        c.press("f5").wait(30)
        c.up("shift").wait(10)
        c.up("page").wait(20)

        self.assertTrue(self.env.sequencer.isModalPageTop)

    def _trigger_steps_context_action(self, function_button):
        c = self.controller
        c.down("page").wait(10)
        c.down("shift").wait(10)
        c.press(function_button).wait(30)
        c.up("shift").wait(10)
        c.up("page").wait(20)

    def _run_init_steps_from_generator_select(self, row_index):
        c = self.controller
        self._open_generator_select_from_steps()
        for _ in range(row_index):
            c.right().wait(10)
        c.encoder().wait(40)

    def _set_single_persistent_selection(self, step_button):
        c = self.controller
        c.down("shift").wait(10)
        c.press(step_button).wait(10)
        c.up("shift").wait(10)

    def _selection_changed_indices(self, before_values, after_values):
        changed = []
        for idx, (before, after) in enumerate(zip(before_values, after_values)):
            if before != after:
                changed.append(idx)
        return changed

    def _configure_note_track(self):
        p = self.env.sequencer.model.project
        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        return p

    def _configure_stochastic_track(self):
        p = self.env.sequencer.model.project
        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Stochastic)
        return p

    def _configure_arp_track(self):
        p = self.env.sequencer.model.project
        p.selectedTrackIndex = 0
        p.setTrackMode(0, p.tracks[0].TrackMode.Arp)
        return p

    def test_note_init_layer_and_init_steps_selection_and_fallback(self):
        p = self._configure_note_track()
        c = self.controller

        c.selectPage("steps")
        p.selectedNoteSequenceLayer = p.selectedNoteSequence.Layer.Note

        sequence = p.selectedNoteSequence
        for idx, note in enumerate([5, 7, 9, 11]):
            step = sequence.steps[idx]
            step.gate = True
            step.length = 3 + idx
            step.note = note

        before_notes = [sequence.steps[i].note for i in range(4)]
        before_gates = [sequence.steps[i].gate for i in range(4)]
        before_lengths = [sequence.steps[i].length for i in range(4)]

        # Init Layer from steps context menu: selection-aware and layer-only.
        self._set_single_persistent_selection("step1")
        self._trigger_steps_context_action("f1")

        after_init_layer_notes = [sequence.steps[i].note for i in range(4)]
        after_init_layer_gates = [sequence.steps[i].gate for i in range(4)]
        after_init_layer_lengths = [sequence.steps[i].length for i in range(4)]

        changed_note_indices = self._selection_changed_indices(before_notes, after_init_layer_notes)
        self.assertEqual(len(changed_note_indices), 1)
        selected_index = changed_note_indices[0]
        self.assertEqual(after_init_layer_notes[selected_index], 0)
        self.assertEqual(after_init_layer_gates[selected_index], before_gates[selected_index])
        self.assertEqual(after_init_layer_lengths[selected_index], before_lengths[selected_index])

        for idx in range(4):
            if idx == selected_index:
                continue
            self.assertEqual(after_init_layer_notes[idx], before_notes[idx])
            self.assertEqual(after_init_layer_gates[idx], before_gates[idx])
            self.assertEqual(after_init_layer_lengths[idx], before_lengths[idx])

        # Prepare a non-default state again.
        for idx, note in enumerate([13, 15, 17, 19]):
            step = sequence.steps[idx]
            step.gate = True
            step.length = 5 + idx
            step.note = note

        # Init Steps from GEN chooser with one selected step: all layers on selected step only.
        self._run_init_steps_from_generator_select(4)  # Note mode chooser: row 4 = Init Steps

        gates_after_selected_init_steps = [sequence.steps[i].gate for i in range(4)]
        notes_after_selected_init_steps = [sequence.steps[i].note for i in range(4)]
        lengths_after_selected_init_steps = [sequence.steps[i].length for i in range(4)]

        self.assertFalse(gates_after_selected_init_steps[selected_index])
        self.assertEqual(notes_after_selected_init_steps[selected_index], 0)
        self.assertNotEqual(lengths_after_selected_init_steps[selected_index], 5 + selected_index)

        for idx in range(4):
            if idx == selected_index:
                continue
            self.assertTrue(gates_after_selected_init_steps[idx])
            self.assertEqual(notes_after_selected_init_steps[idx], 13 + idx * 2)
            self.assertEqual(lengths_after_selected_init_steps[idx], 5 + idx)

        # Clear selection, mutate first four steps again, then Init Steps fallback must target whole track.
        self._set_single_persistent_selection("step1")  # toggle same step -> no selection
        for idx, note in enumerate([21, 23, 25, 27]):
            step = sequence.steps[idx]
            step.gate = True
            step.length = 10 + idx
            step.note = note

        self._run_init_steps_from_generator_select(4)

        for idx in range(4):
            self.assertFalse(sequence.steps[idx].gate)
            self.assertEqual(sequence.steps[idx].note, 0)
            self.assertNotEqual(sequence.steps[idx].length, 10 + idx)

    def test_stochastic_init_layer_init_steps_and_init_seq(self):
        p = self._configure_stochastic_track()
        c = self.controller

        c.selectPage("steps")
        p.selectedStochasticSequenceLayer = p.selectedStochasticSequence.Layer.NoteVariationProbability

        sequence = p.selectedStochasticSequence
        for idx, base_note in enumerate([4, 6, 8, 10]):
            step = sequence.steps[idx]
            step.gate = True
            step.note = base_note
            step.noteVariationProbability = 5 + idx

        before_var_prob = [sequence.steps[i].noteVariationProbability for i in range(4)]
        before_gates = [sequence.steps[i].gate for i in range(4)]
        before_notes = [sequence.steps[i].note for i in range(4)]

        # Init Layer (Note Prob) should not clear full steps.
        self._set_single_persistent_selection("step1")
        self._trigger_steps_context_action("f1")

        after_var_prob = [sequence.steps[i].noteVariationProbability for i in range(4)]
        changed_indices = self._selection_changed_indices(before_var_prob, after_var_prob)
        self.assertEqual(len(changed_indices), 1)
        selected_index = changed_indices[0]
        self.assertEqual(after_var_prob[selected_index], 0)
        self.assertEqual(sequence.steps[selected_index].gate, before_gates[selected_index])
        self.assertEqual(sequence.steps[selected_index].note, before_notes[selected_index])

        # Init Steps from GEN chooser with selection: full reset on selected step, with chromatic note default.
        self._run_init_steps_from_generator_select(2)  # Non-note chooser: row 2 = Init Steps
        self.assertFalse(sequence.steps[selected_index].gate)
        self.assertEqual(sequence.steps[selected_index].note, selected_index)

        # No-selection fallback: Init Steps must affect full track.
        self._set_single_persistent_selection("step1")  # clear persisted selection
        for idx in range(4):
            sequence.steps[idx].gate = True
            sequence.steps[idx].note = 40 + idx
            sequence.steps[idx].noteVariationProbability = 9

        self._run_init_steps_from_generator_select(2)
        for idx in range(4):
            self.assertFalse(sequence.steps[idx].gate)
            self.assertEqual(sequence.steps[idx].note, idx)
            self.assertEqual(sequence.steps[idx].noteVariationProbability, 0)

        # Init Seq from SEQ page restores deterministic sequence defaults.
        sequence.lowOctaveRange = -1
        sequence.highOctaveRange = 3
        c.selectPage("sequence")
        self._trigger_steps_context_action("f1")
        self.assertEqual(sequence.lowOctaveRange, -3)
        self.assertEqual(sequence.highOctaveRange, 0)
        for idx in range(12):
            self.assertEqual(sequence.steps[idx].note, idx)

    def test_arp_init_layer_init_steps_and_init_seq(self):
        p = self._configure_arp_track()
        c = self.controller

        c.selectPage("steps")
        p.selectedArpSequenceLayer = p.selectedArpSequence.Layer.Note

        sequence = p.selectedArpSequence
        for idx, base_note in enumerate([2, 4, 6, 8]):
            step = sequence.steps[idx]
            step.gate = True
            step.note = base_note
            step.length = 2 + idx

        before_notes = [sequence.steps[i].note for i in range(4)]
        before_gates = [sequence.steps[i].gate for i in range(4)]
        before_lengths = [sequence.steps[i].length for i in range(4)]

        # Init Layer on NOTE should restore arp-note chromatic default and keep other layers.
        self._set_single_persistent_selection("step1")
        self._trigger_steps_context_action("f1")

        after_notes = [sequence.steps[i].note for i in range(4)]
        changed_indices = self._selection_changed_indices(before_notes, after_notes)
        self.assertEqual(len(changed_indices), 1)
        selected_index = changed_indices[0]
        self.assertEqual(after_notes[selected_index], selected_index)
        self.assertEqual(sequence.steps[selected_index].gate, before_gates[selected_index])
        self.assertEqual(sequence.steps[selected_index].length, before_lengths[selected_index])

        # Init Steps with selection: full reset on selected step, chromatic default preserved.
        self._run_init_steps_from_generator_select(2)
        self.assertFalse(sequence.steps[selected_index].gate)
        self.assertEqual(sequence.steps[selected_index].note, selected_index)

        # No-selection fallback: full track.
        self._set_single_persistent_selection("step1")  # clear persisted selection
        for idx in range(4):
            sequence.steps[idx].gate = True
            sequence.steps[idx].note = 50 + idx
            sequence.steps[idx].length = 9

        self._run_init_steps_from_generator_select(2)
        for idx in range(4):
            self.assertFalse(sequence.steps[idx].gate)
            self.assertEqual(sequence.steps[idx].note, idx)

        # Init Seq from SEQ page restores sequence defaults.
        sequence.lowOctaveRange = -2
        sequence.highOctaveRange = 2
        c.selectPage("sequence")
        self._trigger_steps_context_action("f1")
        self.assertEqual(sequence.lowOctaveRange, 0)
        self.assertEqual(sequence.highOctaveRange, 0)
        for idx in range(12):
            self.assertEqual(sequence.steps[idx].note, idx)
