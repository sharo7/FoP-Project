#include "sensing_commands.h"

double distanceToSprite(const Sprite& sprite1, const Sprite& sprite2)
{
    return sqrt((sprite1.xCenter - sprite2.xCenter) * (sprite1.xCenter - sprite2.xCenter) +
        (sprite1.yCenter - sprite2.yCenter) * (sprite1.yCenter - sprite2.yCenter));
}

double distanceToMouse(const Sprite& sprite)
{
    int xMouse;
    int yMouse;
    SDL_GetMouseState(&xMouse, &yMouse);
    return sqrt((sprite.xCenter - xMouse) * (sprite.xCenter - xMouse) +
        (sprite.yCenter - yMouse) * (sprite.yCenter - yMouse));
}

double timer()
{
    return (SDL_GetTicks() - resetTime) / 1000.0;
}

void resetTimer()
{
    resetTime = SDL_GetTicks();
}

bool touchingMousePointer(const Sprite& sprite)
{
    int xMouse;
    int yMouse;
    SDL_GetMouseState(&xMouse, &yMouse);
    bool touching = false;
    if (xMouse >= sprite.xCenter - sprite.costumeWidth / 2.0 && xMouse <= sprite.xCenter + sprite.costumeWidth / 2.0
        && yMouse <= sprite.yCenter + sprite.costumeHeight / 2.0 && yMouse >= sprite.yCenter - sprite.costumeHeight /
        2.0)
        touching = true;
    return touching;
}

bool touchingSprite(const Sprite& sprite1, const Sprite& sprite2)
{
    SDL_Rect sprite1Rect;
    SDL_Rect sprite2Rect;
    sprite1Rect.x = sprite1.xCenter - sprite1.costumeWidth / 2.0;
    sprite1Rect.y = sprite1.yCenter - sprite1.costumeHeight / 2.0;
    sprite1Rect.w = sprite1.costumeWidth;
    sprite1Rect.h = sprite1.costumeHeight;
    sprite1Rect.x = sprite1.xCenter - sprite1.costumeWidth / 2.0;
    sprite1Rect.y = sprite1.yCenter - sprite1.costumeHeight / 2.0;
    sprite1Rect.w = sprite1.costumeWidth;
    sprite1Rect.h = sprite1.costumeHeight;
    bool touching = SDL_HasIntersection(&sprite1Rect, &sprite2Rect);
    return touching;
}

bool touchingEdge(const Sprite& sprite)
{
    if (sprite.xCenter - sprite.costumeWidth / 2.0 <= gameArea.x)
        return true;
    if (sprite.xCenter + sprite.costumeWidth / 2.0 >= gameArea.x + gameArea.w)
        return true;
    if (sprite.yCenter - sprite.costumeHeight / 2.0 <= gameArea.y)
        return true;
    if (sprite.yCenter + sprite.costumeHeight / 2.0 >= gameArea.y + gameArea.h)
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
    if ((SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0)
        return true;
    return false;
}

bool keyPressed(const string& keyName)
{
    const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
    SDL_Scancode keyCode = SDL_SCANCODE_UNKNOWN;
    if (keyName == "any")
    {
        for (int i = 0; i < SDL_NUM_SCANCODES; i++)
            if (keyboardState[i] == 1)
                return true;
        return false;
    }
    else if (keyName == "space")
        keyCode = SDL_SCANCODE_SPACE;
    else if (keyName == "enter")
        keyCode = SDL_SCANCODE_RETURN;
    else if (keyName == "up arrow")
        keyCode = SDL_SCANCODE_UP;
    else if (keyName == "left arrow")
        keyCode = SDL_SCANCODE_LEFT;
    else if (keyName == "right arrow")
        keyCode = SDL_SCANCODE_RIGHT;
    else if (keyName == "down arrow")
        keyCode = SDL_SCANCODE_DOWN;
    else if (keyName.length() == 1) //the input is a letter or a number
    {
        char key = keyName[0];
        if ('a' <= key && key <= 'z')
            keyCode = static_cast<SDL_Scancode>(SDL_SCANCODE_A + (key - 'a'));
        else if ('A' <= key && key <= 'Z')
            keyCode = static_cast<SDL_Scancode>(SDL_SCANCODE_A + (key - 'A'));
        else if ('0' <= key && key <= '9')
            keyCode = static_cast<SDL_Scancode>(SDL_SCANCODE_A + (key - '0'));
    }
    if (keyCode != SDL_SCANCODE_UNKNOWN)
        if (keyboardState[keyCode] == 1)
            return true;
    return false;
}

void setDragMode(Sprtie& sprite, bool draggable)
{
    sprite.draggable = draggable;
    if (!draggable)
        sprite.dragging = false; // handles the situation where the sprite is being dragged and in the middle of it
    //the drag mode is turned off
}
