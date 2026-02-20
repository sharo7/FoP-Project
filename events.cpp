#include "events.h"
#include "interpreter.h"

bool whenGreenFlagClicked(bool isClicked) {
    return isClicked;
}

bool whenSomeKeyPressed(const string &key) {
    bool isItPressed = false;
    return isItPressed;
}

bool whenThisSpriteClicked(bool isClicked) {
    return isClicked;
}

bool whenXisGreaterThanY(const Variable &X, double Y) {
    if (X.value.numVal > Y)
        return true;
    return false;
}
