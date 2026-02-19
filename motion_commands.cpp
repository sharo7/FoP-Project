#include "motion_commands.h"

void move(Sprite &sprite, double n)
{
    sprite.xCenter+=n*cos(sprite.direction*M_PI/180);
    sprite.yCenter-=n*sin(sprite.direction*M_PI/180);
}
void turnAnticlockwise(Sprite &sprite, double theta)
{
    sprite.direction+=theta;
    correctDirRange(sprite);
}
void turnClockwise(Sprite &sprite, double theta)
{
    sprite.direction-=theta;
    correctDirRange(sprite);
}
void pointInDirection(Sprite &sprite, double theta)
{
    sprite.direction=theta;
    correctDirRange(sprite);
}
void goToXY(Sprite &sprite, double x, double y)
{
    sprite.xCenter=x+gameArea.x;
    sprite.yCenter=y+gameArea.y;
}
void changeXBy(Sprite &sprite, double dx)
{
    sprite.xCenter+=dx;
}
void changeYBy(Sprite &sprite, double dy)
{
    sprite.yCenter+=dy;
}
void goToMousePointer(Sprite &sprite)
{
    int xMouse;
    int yMouse;
    SDL_GetMouseState(&xMouse, &yMouse);
    sprite.xCenter=xMouse;
    sprite.yCenter=yMouse;
}
void goToRandomPosition(Sprite &sprite)
{
    static mt19937 gen(time(nullptr));
    uniform_real_distribution<> x(gameArea.x, gameArea.x+gameArea.w);
    uniform_real_distribution<> y(gameArea.y, gameArea.y+gameArea.h);
    sprite.xCenter=x(gen);
    sprite.yCenter=y(gen);
}
void ifOnEdgeBounce(Sprite &sprite)
{
    if (sprite.xCenter-sprite.costumeWidth/2.0<gameArea.x||sprite.xCenter+sprite.costumeWidth/2.0>gameArea.x+gameArea.w)
        pointInDirection(sprite, -sprite.direction);
    if (sprite.yCenter-sprite.costumeHeight/2.0<gameArea.y||sprite.yCenter+sprite.costumeHeight/2.0>gameArea.y+gameArea.h)
        pointInDirection(sprite, 180-sprite.direction);
}