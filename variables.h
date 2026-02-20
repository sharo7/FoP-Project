#ifndef SCRATCH_VARIABLES_H
#define SCRATCH_VARIABLES_H

#include <bits/stdc++.h>
#include "interpreter.h"

using namespace std;

int processValue(const Value &v);

void createVariable(const string &name, bool isGlobal, int ownerSpriteId);

void setVariableValue(const string &name, int requesterSpriteId, const Value &newValue);

void changeVariableValue(const string &name, int requesterSpriteId, const Value &newValue);

void showVariable(const string &name, int requesterSpriteId);

void hideVariable(const string &name, int requesterSpriteId);

template<typename T>
T getVariableValue(const string &name, int requesterSpriteId);

#endif //SCRATCH_VARIABLES_H
