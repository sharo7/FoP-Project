#ifndef SCRATCH_VARIABLES_H
#define SCRATCH_VARIABLES_H

#include <bits/stdc++.h>

using namespace std;

struct Variable {
    string name;
    double value = 0;
    bool isGlobal = true;
    int ownerSpriteId = -1;
    //if the variable is global then it's ownerSpriteId is -1.
    //otherwise it will be the Id of the sprite which is selected right now.
    bool show = false;
    //it determines if the variable should be shown in a table at the top of the main program
};

map<string, Variable> variables; // first is Variable's name

void createVariable(const string &name, bool isGlobal, int ownerSpriteId);

void setVariableValue(const string &name, int requesterSpriteId, double newValue);

void changeVariableValue(const string &name, int requesterSpriteId, double newValue);

void showVariable(const string &name, int requesterSpriteId);

void hideVariable(const string &name, int requesterSpriteId);

double getVariableValue(const string &name, int requesterSpriteId);

#endif //SCRATCH_VARIABLES_H
