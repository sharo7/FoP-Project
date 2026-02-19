#include "sensing_commands.h"
double distanceToSprite(const Sprite &sprite1, const Sprite &sprite2)
{
    return sqrt((sprite1.xCenter-sprite2.xCenter)*(sprite1.xCenter-sprite2.xCenter)+
        (sprite1.yCenter-sprite2.yCenter)*(sprite1.yCenter-sprite2.yCenter));
}
double distanceToMouse(const Sprite &sprite)
{
    int xMouse;
    int yMouse;
    SDL_GetMouseState(&xMouse, &yMouse);
    return sqrt((sprite.xCenter-xMouse)*(sprite.xCenter-xMouse)+
        (sprite.yCenter-yMouse)*(sprite.yCenter-yMouse));
}
double timer()
{
    return (SDL_GetTicks()-resetTime)/1000.0;
}
void resetTimer()
{
    resetTime=SDL_GetTicks();
}
bool touchingMousePointer(const Sprite &sprite)
{
    int xMouse;
    int yMouse;
    SDL_GetMouseState(&xMouse, &yMouse);
    bool touching=false;
    if (xMouse>=sprite.xCenter-sprite.costumeWidth/2&&xMouse<=sprite.xCenter+sprite.costumeWidth/2
        &&yMouse<=sprite.yCenter+sprite.costumeHeight/2&&yMouse>=sprite.yCenter-sprite.costumeHeight/2)
        touching=true;
    return touching;
}
bool touchingSprite(const Sprite &sprite1, const Sprite &sprite2)
{
    SDL_Rect sprite1Rect;
    SDL_Rect sprite2Rect;
    sprite1Rect.x=sprite1.xCenter-sprite1.costumeWidth/2;
    sprite1Rect.y=sprite1.yCenter-sprite1.costumeHeight/2;
    sprite1Rect.w=sprite1.costumeWidth;
    sprite1Rect.h=sprite1.costumeHeight;
    sprite1Rect.x=sprite1.xCenter-sprite1.costumeWidth/2;
    sprite1Rect.y=sprite1.yCenter-sprite1.costumeHeight/2;
    sprite1Rect.w=sprite1.costumeWidth;
    sprite1Rect.h=sprite1.costumeHeight;
    bool touching=SDL_HasIntersection(&sprite1Rect, &sprite2Rect);
    return touching;
}
bool touchingEdge(const Sprite &sprite)
{
    if (sprite.xCenter-sprite.costumeWidth/2<=gameArea.x)
        return true;
    if (sprite.xCenter+sprite.costumeWidth/2>=gameArea.x+gameArea.w)
        return true;
    if (sprite.yCenter-sprite.costumeHeight/2<=gameArea.y)
        return true;
    if (sprite.yCenter+sprite.costumeHeight/2>=gameArea.y+gameArea.h)
        return true;
    return false;
}
int mouseX()
{
    int xMouse;
    SDL_GetMouseState(&xMouse, nullptr);
    return xMouse;
}
int mouseY()
{
    int yMouse;
    SDL_GetMouseState(nullptr, &yMouse);
    return yMouse;
}
bool mouseDown()
{
    if ((SDL_GetMouseState(nullptr, nullptr)&SDL_BUTTON(SDL_BUTTON_LEFT))!=0)
        return true;
    return false;
}