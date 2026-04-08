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
