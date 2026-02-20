#include "variables.h"

int processValue(const Value &v) {
    switch (v.type) {
        case NUMBER:
            return 1;
        case STRING:
            return 2;
        case BOOLEAN:
            return 3;
        case SPRITE:
            return 4;
    }
    return 0;
}

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

void setVariableValue(const string &name, int requesterSpriteId, const Value &newValue) {
    if (variables.find(name) != variables.end()) {
        if (variables[name].isGlobal) {
            variables[name].value = newValue;
            return;
        }
        if (variables[name].ownerSpriteId == requesterSpriteId)
            variables[name].value = newValue;
        else {
            //should show a message that the variable doesn't belong to this sprite!
        }
    }
}

void changeVariableValue(const string &name, int requesterSpriteId, const Value &newValue) {
    if (variables.find(name) != variables.end()) {
        if (variables[name].isGlobal) {
            variables[name].value = newValue;
            return;
        }
        if (variables[name].ownerSpriteId == requesterSpriteId)
            variables[name].value = newValue;
        else {
            //should show a message that the variable doesn't belong to this sprite!
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
            //should show a message that the variable doesn't belong to this sprite!
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
            //should show a message that the variable doesn't belong to this sprite!
        }
    }
}

template<typename T>
T getVariableValue(const string &name, int requesterSpriteId) {
    int variableIdent = processValue(variables[name].value);
    if (variables.find(name) != variables.end()) {
        if (variables[name].isGlobal || variables[name].ownerSpriteId == requesterSpriteId) {
            if (variableIdent == 1)
                return variables[name].value.numVal;
            if (variableIdent == 2)
                return variables[name].value.strVal;
            if (variableIdent == 3)
                return variables[name].value.boolVal;
        }
    }
    return NAN;
}
