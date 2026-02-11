#ifndef SPRITE_H
#define SPRITE_H
#include <bits/stdc++.h>
#include <SDL2/SDL.h>
using namespace std;
struct Sprite
{
    double xCenter, yCenter;//sprite's center coordinates on the sdl default coordination system
    double direction;//the angle in degrees
    //direction 0 is pointing right and rotation direction is anticlockwise
    bool visible;
    SDL_Texture* spriteTexture;
    int spriteWidth, spriteHeight;//SDL_QueryTexture(spriteTexture, nullptr, nullptr, &spriteWidth, &spriteHeight);
};
void correctDirRange(Sprite& sprite);//valid range for the angle is -180 to 180 degrees
Sprite createSprite(SDL_Texture* texture, int windowWidth, int windowLength);
#endif //SPRITE_H