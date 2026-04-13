include_directories(../../../apps/sequencer)

register_test(TestCurve TestCurve.cpp)
register_test(TestGeneratorSemantics TestGeneratorSemantics.cpp)
register_test(TestNoteSequence TestNoteSequence.cpp)
register_test(TestScale TestScale.cpp)
register_test(TestVoltageModeRegression TestVoltageModeRegression.cpp)

target_link_libraries(TestGeneratorSemantics sequencer_shared)
target_link_libraries(TestVoltageModeRegression sequencer_shared)
