#include "looks_commands.h"

void show(Sprite &sprite) {
    sprite.visible = true;
}
void hide(Sprite &sprite) {
    sprite.visible = false;
}
void addCostume(Sprite &sprite, SDL_Texture* costumeTexture, const string &costumeName)
{
    sprite.spriteCostumes.push_back({costumeName, costumeTexture});
}
void nextCostume(Sprite &sprite)
{
    sprite.currentCostumeIndex++;
    if (sprite.currentCostumeIndex==sprite.spriteCostumes.size())
        sprite.currentCostumeIndex=0;
    SDL_QueryTexture(sprite.spriteCostumes[sprite.currentCostumeIndex].costumeTexture,
        nullptr, nullptr, &sprite.costumeWidth, &sprite.costumeHeight);
}
void switchCostumeTo(Sprite &sprite, const string &costumeName)
{
    for (int i=0; i<sprite.spriteCostumes.size(); i++)
    {
        if (sprite.spriteCostumes[i].costumeName==costumeName)
        {
            sprite.currentCostumeIndex=i;
            SDL_QueryTexture(sprite.spriteCostumes[sprite.currentCostumeIndex].costumeTexture,
                nullptr, nullptr, &sprite.costumeWidth, &sprite.costumeHeight);
            return;
        }
    }
}
void setSizeTo(Sprite &sprite, double scalePercentage)
{
    if (scalePercentage<0)
        scalePercentage=0;
    sprite.spriteSize=scalePercentage;
}
void changeSizeBy(Sprite &sprite, double scalePercentage)
{
    sprite.spriteSize+=scalePercentage;
    if (sprite.spriteSize<0)
        sprite.spriteSize=0;
}