#include "SequencerApp.h"
#include "model/Model.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void register_project(py::module &m);

void register_sequencer(py::module &m) {
    // ------------------------------------------------------------------------
    // Sequencer
    // ------------------------------------------------------------------------

    py::class_<SequencerApp> sequencer(m, "Sequencer");
    sequencer
        .def_property_readonly("model", [] (SequencerApp &app) { return &app.model; })
        .def_property_readonly("uiPageKind", [] (SequencerApp &app) { return int(app.ui.uiPageKind()); })
        .def_property_readonly("isGeneratorPageTop", [] (SequencerApp &app) { return app.ui.isGeneratorPageTop(); })
        .def_property_readonly("isNoteSequenceEditPageTop", [] (SequencerApp &app) { return app.ui.isNoteSequenceEditPageTop(); })
        .def_property_readonly("isModalPageTop", [] (SequencerApp &app) { return app.ui.isModalPageTop(); })
        .def_property_readonly("launchpadControllerConnectedForTest", [] (SequencerApp &app) {
            return app.ui.launchpadControllerConnectedForTest();
        })
        .def_property_readonly("launchpadGeneratorsModeActiveForTest", [] (SequencerApp &app) {
            return app.ui.launchpadGeneratorsModeActiveForTest();
        })
        .def("connectLaunchpadForTest", [] (SequencerApp &app, uint16_t vendorId, uint16_t productId) {
            app.ui.connectLaunchpadForTest(vendorId, productId);
        }, py::arg("vendorId") = 0x1235, py::arg("productId") = 0x0069)
        .def("disconnectLaunchpadForTest", [] (SequencerApp &app) {
            app.ui.disconnectLaunchpadForTest();
        })
    ;

    // ------------------------------------------------------------------------
    // Model
    // ------------------------------------------------------------------------

    py::class_<Model> model(m, "Model");
    model
        .def_property_readonly("project", [] (Model &model) { return &model.project(); })
    ;

    register_project(m);
}
