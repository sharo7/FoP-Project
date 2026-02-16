#include "sprite.h"
void correctDirRange(Sprite& sprite)
{
    while (sprite.direction>180)
        sprite.direction-=360;
    while (sprite.direction<-180)
        sprite.direction+=360;
}
Sprite createSprite(SDL_Texture* costumeTexture, const string &costumeName, int windowWidth, int windowLength)
{
    Sprite sprite;
    sprite.visible=true;
    sprite.spriteCostumes.push_back({costumeName, costumeTexture});
    sprite.currentCostumeIndex=0;
    SDL_QueryTexture(costumeTexture, nullptr, nullptr,
        &sprite.costumeWidth, &sprite.costumeHeight);
    sprite.direction=0;
    sprite.xCenter=windowWidth/2.0;
    sprite.yCenter=windowLength/2.0;
    sprite.spriteSize=100;
    return sprite;
}
void drawSprite(SDL_Renderer* renderer, const Sprite &sprite)
{
    if (sprite.visible==false)
        return;
    SDL_Texture* currentTexture=sprite.spriteCostumes[sprite.currentCostumeIndex].costumeTexture;
    SDL_Rect spriteRect;
    spriteRect.w=sprite.costumeWidth*sprite.spriteSize/100;
    spriteRect.h=sprite.costumeHeight*sprite.spriteSize/100;
    spriteRect.x=sprite.xCenter-(sprite.costumeWidth*sprite.spriteSize/100)/2.0;
    spriteRect.y=sprite.yCenter-(sprite.costumeHeight*sprite.spriteSize/100)/2.0;
    SDL_RenderCopyEx(renderer, currentTexture, nullptr,
        &spriteRect, -sprite.direction, nullptr, SDL_FLIP_NONE );
}