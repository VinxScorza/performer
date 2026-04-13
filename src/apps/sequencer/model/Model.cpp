#include "Model.h"

Model::Model() :
    _clipBoard(_project)
{}

void Model::init() {
    _project.clear();
    _clipBoard.clear();
    _knobPad16Armed = false;
}
