#include "Screensaver.h"

void Screensaver::on() {
    _screenSaved = true;
    _canvas.screensaver();
}

void Screensaver::off() {
    _screenSaved = false;
    _buttonPressed = false;
    setScreenOnTicks(0);
}

bool Screensaver::shouldBeOn() {
    // TODO Is tick comparison correct? Seems roughly correct...
    return _screenOffAfter > 0 && !_buttonPressed && _screenOnTicks > _screenOffAfter;
}

void Screensaver::consumeKey(KeyEvent &event) {
    consumeKey(event, event.key());
}

void Screensaver::consumeKey(KeyPressEvent &event) {
    consumeKey(event, event.key());
}

void Screensaver::consumeKey(Event &event, Key key) {
    if (_wakeKey != Key::None && key.code() == _wakeKey) {
        event.consume();
        if (event.type() == Event::Type::KeyUp) {
            _wakeKey = Key::None;
        }
        return;
    }

    if (_screenSaved) {
        if (_wakeMode == 1) { // required
            event.consume();
            if (event.type() == Event::Type::KeyDown || event.type() == Event::Type::KeyPress) {
                _wakeKey = key.code();
            }
        }

        off();
        return;
    }

    // Don't turn on screensaver if button is held
    if (event.type() == Event::Type::KeyDown) {
        _buttonPressed = true;
    } else if (event.type() == Event::Type::KeyUp) {
        _buttonPressed = false;
    }

    if (event.type() == Event::Type::KeyPress) {
        off();
    }
}

void Screensaver::consumeEncoder(EncoderEvent &event) {
    if (_screenSaved) {
        if (_wakeMode == 1) { // required
            event.consume();
        }

        off();
        return;
    }

    off();
}

void Screensaver::setScreenOnTicks(uint32_t ticks) {
    _screenOnTicks = ticks;
}

void Screensaver::incScreenOnTicks(uint32_t ticks) {
    if (_screenOffAfter > 0) { // Prevent flickering when setting screensaver in settings
        _screenOnTicks += ticks;
    }
}
