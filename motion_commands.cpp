#include "motion_commands.h"

void move(Sprite &sprite, double n) {
    sprite.xCenter += n * cos(sprite.direction * M_PI / 180);
    sprite.yCenter -= n * sin(sprite.direction * M_PI / 180);
}

void turnAnticlockwise(Sprite &sprite, double theta) {
    sprite.direction += theta;
    correctDirRange(sprite);
}

void turnClockwise(Sprite &sprite, double theta) {
    sprite.direction -= theta;
    correctDirRange(sprite);
}

void pointInDirection(Sprite &sprite, double theta) {
    sprite.direction = theta;
    correctDirRange(sprite);
}

void goToXY(Sprite &sprite, double x, double y) {
    sprite.xCenter = x;
    sprite.yCenter = y;
}

void changeXBy(Sprite &sprite, double dx) {
    sprite.xCenter += dx;
}

void changeYBy(Sprite &sprite, double dy) {
    sprite.yCenter += dy;
}

void goToMousePointer(Sprite &sprite, int xMouse, int yMouse) {
    sprite.xCenter = xMouse;
    sprite.yCenter = yMouse;
}

void goToRandomPosition(Sprite &sprite, int windowWidth, int windowLength) {
    static mt19937 gen(time(nullptr));
    uniform_real_distribution<> x(sprite.spriteWidth / 2.0, windowWidth - sprite.spriteWidth / 2.0);
    uniform_real_distribution<> y(sprite.spriteHeight / 2.0, windowLength - sprite.spriteHeight / 2.0);
    sprite.xCenter = x(gen);
    sprite.yCenter = y(gen);
}

void ifOnEdgeBounce(Sprite &sprite, int windowWidth, int windowLength) {
    if (sprite.xCenter - sprite.spriteWidth / 2.0 < 0 || sprite.xCenter + sprite.spriteWidth / 2.0 > windowWidth)
        pointInDirection(sprite, -sprite.direction);
    if (sprite.yCenter - sprite.spriteHeight / 2.0 < 0 || sprite.yCenter + sprite.spriteHeight / 2.0 > windowLength)
        pointInDirection(sprite, 180 - sprite.direction);
}
