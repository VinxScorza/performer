import testframework as tf

class ProjectPageTest(tf.UiTest):

    def test_track_select_enters_steps_from_project_like_pages(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.setTrackMode(0, p.tracks[0].TrackMode.Note)
        pages = ("project", "layout", "routing", "midiout", "userscale", "clock")

        for page in pages:
            c.selectPage(page)
            c.press("track1").wait(20)
            self.assertEqual(p.selectedTrackIndex, 0, f"{page}: selected track")
            self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop, f"{page}: jump to Steps")

    def test_track_double_click_enters_steps_from_operational_page(self):
        c = self.controller
        p = self.env.sequencer.model.project

        p.setTrackMode(1, p.tracks[1].TrackMode.Note)

        c.selectPage("track")
        c.press("track2").press("track2").wait(20)

        self.assertEqual(p.selectedTrackIndex, 1)
        self.assertTrue(self.env.sequencer.isNoteSequenceEditPageTop)

    def test_midi_input_event_filter_defaults_to_all_enabled(self):
        p = self.env.sequencer.model.project
        event = tf.sequencer.Project.MidiInputEvent

        self.assertTrue(p.midiInputEventEnabled(event.Notes))
        self.assertTrue(p.midiInputEventEnabled(event.ControlChange))
        self.assertTrue(p.midiInputEventEnabled(event.ProgramChange))
        self.assertTrue(p.midiInputEventEnabled(event.PitchBend))
        self.assertTrue(p.midiInputEventEnabled(event.Aftertouch))

    def test_midi_input_cc_filter_blocks_routing_cc(self):
        c = self.controller
        p = self.env.sequencer.model.project
        route = p.routing.routes[0]

        route.target = tf.sequencer.Routing.Target.Tempo
        route.source = tf.sequencer.Routing.Source.Midi
        route.min = (110 - 1) / (1000 - 1)
        route.max = (210 - 1) / (1000 - 1)
        route.midiSource.event = tf.sequencer.Routing.MidiSource.Event.ControlAbsolute
        route.midiSource.controlNumber = 7

        c.wait(20)
        c.midi(0, tf.core.MidiMessage.makeControlChange(0, 7, 127)).wait(20)
        tempo_after_allowed_cc = p.tempo
        self.assertGreater(tempo_after_allowed_cc, 190)

        p.setMidiInputEventEnabled(tf.sequencer.Project.MidiInputEvent.ControlChange, False)
        c.midi(0, tf.core.MidiMessage.makeControlChange(0, 7, 0)).wait(20)
        self.assertAlmostEqual(p.tempo, tempo_after_allowed_cc, places=1)

        p.setMidiInputEventEnabled(tf.sequencer.Project.MidiInputEvent.ControlChange, True)
        c.midi(0, tf.core.MidiMessage.makeControlChange(0, 7, 0)).wait(20)
        self.assertLess(p.tempo, 130)

    def test_midi_notes_filter_blocks_new_notes_but_allows_note_off(self):
        c = self.controller
        p = self.env.sequencer.model.project
        route = p.routing.routes[0]

        route.target = tf.sequencer.Routing.Target.Tempo
        route.source = tf.sequencer.Routing.Source.Midi
        route.min = (110 - 1) / (1000 - 1)
        route.max = (210 - 1) / (1000 - 1)
        route.midiSource.event = tf.sequencer.Routing.MidiSource.Event.NoteMomentary
        route.midiSource.note = 60

        c.midi(0, tf.core.MidiMessage.makeNoteOn(0, 60, 100)).wait(20)
        self.assertGreater(p.tempo, 190)

        p.setMidiInputEventEnabled(tf.sequencer.Project.MidiInputEvent.Notes, False)
        c.midi(0, tf.core.MidiMessage.makeNoteOff(0, 60, 0)).wait(20)
        self.assertLess(p.tempo, 130)

        c.midi(0, tf.core.MidiMessage.makeNoteOn(0, 60, 100)).wait(20)
        self.assertLess(p.tempo, 130)

    def test_performer_fill_amount_lane_encoder_edits_fill_amount(self):
        c = self.controller
        p = self.env.sequencer.model.project

        initial_tempo = p.tempo
        initial_divisor = p.selectedNoteSequence.divisor
        initial_amount = p.playState.trackFillAmount(0)

        c.selectPage("perf")
        c.down("step1").wait(10)    # T1 fill amount lane
        c.left().wait(10)
        c.up("step1").wait(20)

        self.assertEqual(p.playState.trackFillAmount(0), initial_amount - 10)
        self.assertEqual(p.selectedNoteSequence.divisor, initial_divisor)
        self.assertEqual(p.playState.trackFillDivisorOverride(0), 0)
        self.assertAlmostEqual(p.tempo, initial_tempo, places=1)

    def test_performer_fill_track_encoder_edits_divisor(self):
        c = self.controller
        p = self.env.sequencer.model.project

        initial_tempo = p.tempo
        initial_divisor = p.selectedNoteSequence.divisor
        initial_amount = p.playState.trackFillAmount(0)

        c.selectPage("perf")
        c.down("f4").wait(10)       # FILL
        c.down("step9").wait(10)    # T1 fill lane
        c.right().wait(10)
        self.assertGreater(p.playState.trackFillDivisorOverride(0), initial_divisor)
        self.assertEqual(p.selectedNoteSequence.divisor, initial_divisor)
        c.up("step9").wait(10)
        c.up("f4").wait(20)

        self.assertEqual(p.playState.trackFillDivisorOverride(0), 0)
        self.assertEqual(p.selectedNoteSequence.divisor, initial_divisor)
        self.assertEqual(p.playState.trackFillAmount(0), initial_amount)
        self.assertAlmostEqual(p.tempo, initial_tempo, places=1)

    def test_performer_global_fill_encoder_edits_divisor(self):
        c = self.controller
        p = self.env.sequencer.model.project

        initial_tempo = p.tempo
        initial_divisor = p.selectedNoteSequence.divisor
        initial_amount = p.playState.trackFillAmount(0)

        c.selectPage("perf")
        c.down("f4").wait(10)       # FILL
        c.right().wait(10)
        self.assertGreater(p.playState.trackFillDivisorOverride(0), initial_divisor)
        self.assertEqual(p.selectedNoteSequence.divisor, initial_divisor)
        c.up("f4").wait(20)

        self.assertEqual(p.playState.trackFillDivisorOverride(0), 0)
        self.assertEqual(p.selectedNoteSequence.divisor, initial_divisor)
        self.assertEqual(p.playState.trackFillAmount(0), initial_amount)
        self.assertAlmostEqual(p.tempo, initial_tempo, places=1)

    def test_performer_direct_fill_track_encoder_edits_divisor(self):
        c = self.controller
        p = self.env.sequencer.model.project

        initial_tempo = p.tempo
        initial_divisor = p.selectedNoteSequence.divisor
        initial_amount = p.playState.trackFillAmount(0)

        c.selectPage("perf")
        c.down("step9").wait(10)    # T1 fill lane
        c.right().wait(10)
        self.assertGreater(p.playState.trackFillDivisorOverride(0), initial_divisor)
        self.assertEqual(p.selectedNoteSequence.divisor, initial_divisor)
        c.up("step9").wait(20)

        self.assertEqual(p.playState.trackFillDivisorOverride(0), 0)
        self.assertEqual(p.selectedNoteSequence.divisor, initial_divisor)
        self.assertEqual(p.playState.trackFillAmount(0), initial_amount)
        self.assertAlmostEqual(p.tempo, initial_tempo, places=1)

    def test_performer_fill_track_button_encoder_edits_divisor(self):
        c = self.controller
        p = self.env.sequencer.model.project

        initial_tempo = p.tempo
        initial_divisor = p.selectedNoteSequence.divisor
        initial_amount = p.playState.trackFillAmount(0)

        c.selectPage("perf")
        c.down("f4").wait(10)       # FILL
        c.down("track1").wait(10)
        c.right().wait(10)
        self.assertGreater(p.playState.trackFillDivisorOverride(0), initial_divisor)
        self.assertEqual(p.selectedNoteSequence.divisor, initial_divisor)
        c.up("track1").wait(10)
        c.up("f4").wait(20)

        self.assertEqual(p.playState.trackFillDivisorOverride(0), 0)
        self.assertEqual(p.selectedNoteSequence.divisor, initial_divisor)
        self.assertEqual(p.playState.trackFillAmount(0), initial_amount)
        self.assertAlmostEqual(p.tempo, initial_tempo, places=1)

    def test_performer_modal_fill_track_button_encoder_edits_divisor(self):
        c = self.controller
        p = self.env.sequencer.model.project

        initial_tempo = p.tempo
        initial_divisor = p.selectedNoteSequence.divisor
        initial_amount = p.playState.trackFillAmount(0)

        c.down("perf").wait(10)
        c.down("f4").wait(10)       # FILL
        c.down("track1").wait(10)
        c.right().wait(10)
        self.assertGreater(p.playState.trackFillDivisorOverride(0), initial_divisor)
        self.assertEqual(p.selectedNoteSequence.divisor, initial_divisor)
        c.up("track1").wait(10)
        c.up("f4").wait(10)
        c.up("perf").wait(20)

        self.assertEqual(p.playState.trackFillDivisorOverride(0), 0)
        self.assertEqual(p.selectedNoteSequence.divisor, initial_divisor)
        self.assertEqual(p.playState.trackFillAmount(0), initial_amount)
        self.assertAlmostEqual(p.tempo, initial_tempo, places=1)

    def test_edit_name(self):
        c = self.controller
        p = self.env.sequencer.model.project

        initial_name = p.name

        # edit -> cancel
        c.encoder().press("f4").wait()
        self.assertEqual(p.name, initial_name, "edit -> cancel")

        # edit -> clear -> ok
        c.encoder().press("f3").press("f5").wait()
        self.assertEqual(p.name, "", "edit -> clear -> ok")

        # edit -> a -> b -> c -> ok
        c.encoder().encoder().right().encoder().right().encoder().press("f5").wait()
        self.assertEqual(p.name, "ABC", "edit -> write -> ok")

        # edit -> backspace -> ok
        c.encoder().press("f1").wait().press("f5").wait()
        self.assertEqual(p.name, "AB", "edit -> backspace -> ok")

        # edit -> prev -> prev -> delete -> ok
        c.encoder().press("prev").press("prev").press("f2").press("f5").wait()
        self.assertEqual(p.name, "B", "edit -> prev -> prev -> delete -> ok")

    def test_edit_tempo(self):
        c = self.controller
        p = self.env.sequencer.model.project

        # initial tempo
        initial_tempo = p.tempo

        # select tempo
        c.right()

        # increase
        c.encoder().right().encoder().wait()
        self.assertAlmostEqual(p.tempo, initial_tempo + 1, places=1, msg="increase")

        # decrease
        c.encoder().left().encoder().wait()
        self.assertAlmostEqual(p.tempo, initial_tempo, places=1, msg="decrease")

        # shift + increase
        c.encoder().down("shift").right().up("shift").encoder().wait()
        self.assertAlmostEqual(p.tempo, initial_tempo + 0.1, places=1, msg="shift + increase")

        # shift + decrease
        c.encoder().down("shift").left().up("shift").encoder().wait()
        self.assertAlmostEqual(p.tempo, initial_tempo, places=1, msg="shift + decrease")

    def test_edit_swing(self):
        c = self.controller
        p = self.env.sequencer.model.project

        # initial swing
        initial_swing = p.swing

        # select swing
        c.right().right()

        # increase
        c.encoder().right().encoder().wait()
        self.assertEqual(p.swing, initial_swing + 1, "increase")

        # decrease
        c.encoder().left().encoder().wait()
        self.assertEqual(p.swing, initial_swing, "decrease")

        # shift + increase
        c.encoder().down("shift").right().up("shift").encoder().wait()
        swing_after_shift_up = p.swing
        self.assertTrue(swing_after_shift_up > initial_swing, "shift + increase")
        self.assertTrue(swing_after_shift_up <= initial_swing + 5, "shift + increase step bound")

        # shift + decrease
        c.encoder().down("shift").left().up("shift").encoder().wait()
        self.assertTrue(p.swing < swing_after_shift_up, "shift + decrease")
