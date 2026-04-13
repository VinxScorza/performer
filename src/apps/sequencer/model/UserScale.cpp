#include "UserScale.h"
#include "ProjectVersion.h"

UserScale::Array UserScale::userScales;

const char *UserScale::defaultName(int index) {
    switch (index) {
    case 0: return "INIT1";
    case 1: return "INIT2";
    case 2: return "INIT3";
    case 3: return "INIT4";
    default: return "INIT";
    }
}

UserScale::UserScale() :
    Scale("")
{
    clear();
}

void UserScale::clear() {
    StringUtils::copy(_name, defaultName(-1), sizeof(_name));
    setMode(Mode::Chromatic);
    clearItems();
}

void UserScale::clearItems() {
    setSize(1);
    _items.fill(0);
    if (_mode == Mode::Voltage) {
        _items[1] = 1000;
    }
}

void UserScale::write(VersionedSerializedWriter &writer) const {
    writer.write(_name, NameLength + 1);
    writer.write(_mode);
    writer.write(_size);

    for (int i = 0; i < _size; ++i) {
        writer.write(_items[i]);
    }

    writer.writeHash();
}

bool UserScale::read(VersionedSerializedReader &reader) {
    clear();

    reader.read(_name, NameLength + 1, ProjectVersion::Version5);
    reader.read(_mode);
    reader.read(_size);

    for (int i = 0; i < _size; ++i) {
        reader.read(_items[i]);
    }

    bool success = reader.checkHash();
    if (!success) {
        clear();
    }

    return success;
}
