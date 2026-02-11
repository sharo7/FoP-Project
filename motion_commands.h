#ifndef MOTION_COMMANDS_H
#define MOTION_COMMANDS_H
#include "sprite.h"
void move(Sprite &sprite, double n);// n is the number of pixels
void turnAnticlockwise(Sprite &sprite, double theta);
void turnClockwise(Sprite &sprite, double theta);
void pointInDirection(Sprite &sprite, double theta);
void goToXY(Sprite &sprite, double x, double y);
void changeXBy(Sprite &sprite, double dx);
void changeYBy(Sprite &sprite, double dy);
void goToMousePointer(Sprite &sprite, int xMouse, int yMouse);
void goToRandomPosition(Sprite &sprite, int windowWidth, int windowLength);
void ifOnEdgeBounce(Sprite &sprite, int windowWidth, int windowLength);
#endif //MOTION_COMMANDS_H