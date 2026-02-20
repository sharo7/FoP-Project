#include "control_flow_logic.h"

void wait(unsigned n) {
    this_thread::sleep_for(chrono::milliseconds(n));
}

// the functions which come after aren't really used anywhere,
// and are just defined to be alongside each other and make the project looks cleaner.
// their functionality has been managed in interpreter.

void repeat(unsigned n) {
}

void forever() {
}

bool if_then(bool shouldـexecute) {
    return shouldـexecute;
}

bool waitUntil (bool condition) {
    return condition;
}

void stopAll() {
}

bool if_then_else(bool discriminant) {
    if (discriminant)
        return true;
    return false;
}
// if discriminant is true, then the commands the first commands will be executed,
// otherwise the second commands will be executed.

void repeatUntil(bool condition) {
}
