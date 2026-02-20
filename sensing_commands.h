#ifndef SENSING_COMMANDS_H
#define SENSING_COMMANDS_H
#include "sprite.h"
inline Uint32 resetTime = 0;

double distanceToSprite(const Sprite &sprite1, const Sprite &sprite2);

double distanceToMouse(const Sprite &sprite);

double timer();

void resetTimer();

bool touchingMousePointer(const Sprite &sprite);

bool touchingSprite(const Sprite &sprite1, const Sprite &sprite2);

bool touchingEdge(const Sprite &sprite);

int mouseX();

int mouseY();

bool mouseDown();

bool keyPressed(const string &keyName);

// this function supports space, enter, arrows, all letters and numbers and any key
//that means it returns true if any key has been pressed
void setDragMode(Sprite &sprite, bool draggable);

#endif //SENSING_COMMANDS_H
