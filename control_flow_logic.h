#ifndef SCRATCH_CONTROL_FLOW_LOGIC_H
#define SCRATCH_CONTROL_FLOW_LOGIC_H

#include <bits/stdc++.h>

using namespace std;

void wait(unsigned n);

// the functions which come after aren't really used anywhere,
// and are just defined to be alongside each other and make the project looks cleaner.
// their functionality has been managed in interpreter.

void repeat(unsigned n);

void forever();

bool if_then(bool shouldـexecute);

bool waitUntil(bool condition);

void stopAll();

bool if_then_else(bool discriminant);

// if discriminant is true, then the commands the first commands will be executed,
// otherwise the second commands will be executed.

void repeatUntil(bool condition);

#endif //SCRATCH_CONTROL_FLOW_LOGIC_H
