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
    sprite.graphicEffectEnabled=false;// in the default mode no graphic effect is on
    sprite.colorEffect=0;
    sprite.thinking=false;
    sprite.saying=false;
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
    //In SDL the texture color cant be changed therefore to be able to perform the color effects on the sprite
    // we use a colored transparent rect on the sprite so the illusion of color change is made
    if (sprite.graphicEffectEnabled)
    {
        Uint8 r=static_cast<int>(sprite.colorEffect)%255;
        Uint8 g=static_cast<int>(sprite.colorEffect+85)%255;
        Uint8 b=static_cast<int>(sprite.colorEffect+170)%255;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, r, g, b, 100);
        SDL_RenderFillRect(renderer, &spriteRect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}
void setUpStage(Stage &stage, int x, int y, int w, int h)
{
    stage.stageRect={x, y, w, h};
    stage.currentBackdropIndex=-1;// this means that no backdrop is being selected at first
}
void drawStage(SDL_Renderer* renderer, const Stage &stage)
{
    if (stage.currentBackdropIndex==-1)
        return;
    SDL_RenderCopy(renderer, stage.backdropTexture[stage.currentBackdropIndex],
        nullptr, &stage.stageRect);
}