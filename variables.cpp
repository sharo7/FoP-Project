#include "variables.h"

void createVariable(const string &name, bool isGlobal, int ownerSpriteId) {
    if (variables.find(name) == variables.end()) {
        variables[name] = Variable();
        variables[name].name = name;
        variables[name].isGlobal = isGlobal;
        variables[name].ownerSpriteId = ownerSpriteId;
    } else {
        //should show a message that variable's name already exists!
    }
}

void setVariableValue(const string &name, int requesterSpriteId, double newValue) {
    if (variables.find(name) != variables.end()) {
        if (variables[name].isGlobal) {
            variables[name].value = newValue;
            return;
        }
        if (variables[name].ownerSpriteId == requesterSpriteId)
            variables[name].value = newValue;
        else {
            //shoud show a message that the varaible doesn't belong to this sprite!
        }
    }
}

void changeVariableValue(const string &name, int requesterSpriteId, double newValue) {
    if (variables.find(name) != variables.end()) {
        if (variables[name].isGlobal) {
            variables[name].value = newValue;
            return;
        }
        if (variables[name].ownerSpriteId == requesterSpriteId)
            variables[name].value = newValue;
        else {
            //shoud show a message that the varaible doesn't belong to this sprite!
        }
    }
}

void showVariable(const string &name, int requesterSpriteId) {
    if (variables.find(name) != variables.end()) {
        if (variables[name].isGlobal) {
            variables[name].show = true;
            return;
        }
        if (variables[name].ownerSpriteId == requesterSpriteId)
            variables[name].show = true;
        else {
            //shoud show a message that the varaible doesn't belong to this sprite!
        }
    }
}

void hideVariable(const string &name, int requesterSpriteId) {
    if (variables.find(name) != variables.end()) {
        if (variables[name].isGlobal) {
            variables[name].show = false;
            return;
        }
        if (variables[name].ownerSpriteId == requesterSpriteId)
            variables[name].show = false;
        else {
            //shoud show a message that the varaible doesn't belong to this sprite!
        }
    }
}

double getVariableValue(const string &name, int requesterSpriteId) {
    if (variables.find(name) != variables.end()) {
        if (variables[name].isGlobal)
            return variables[name].value;
        if (variables[name].ownerSpriteId == requesterSpriteId)
            return variables[name].value;
    }
    return NAN;
}
