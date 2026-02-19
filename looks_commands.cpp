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
void clearGraphicEffects(Sprite &sprite)
{
    sprite.graphicEffectEnabled=false;
}
void setColorEffectTo(Sprite &sprite, double value)
{
    sprite.graphicEffectEnabled=true;
    sprite.colorEffect=value;
}
void changeColorEffectBy(Sprite &sprite, double value)
{
    sprite.graphicEffectEnabled=true;
    sprite.colorEffect+=value;
}
void addBackdrop(Stage &stage, SDL_Texture* backdropTexture, const string& backdropName)
{
    stage.backdropTexture.push_back(backdropTexture);
    stage.backdropName.push_back(backdropName);
}
void switchBackdropTo(Stage &stage, const string backdropName)
{
    for (int i=0; i<stage.backdropTexture.size(); i++)
        if (stage.backdropName[i]==backdropName)
        {
            stage.currentBackdropIndex=i;
            return;
        }
}
void nextBackdrop(Stage &stage)
{
    if (stage.backdropTexture.empty())
        return;
    stage.currentBackdropIndex++;

    if (stage.currentBackdropIndex==stage.backdropTexture.size())
        stage.currentBackdropIndex=0;
}
int costumeNumber(const Sprite &sprite)
{
    return sprite.currentCostumeIndex+1;
}
string costumeName(const Sprite &sprite)
{
    return sprite.spriteCostumes[sprite.currentCostumeIndex].costumeName;
}
int backdropNumber(const Stage &stage)
{
    return stage.currentBackdropIndex+1;// if no backdrop is selected the output would be 0
}
string backdropName(const Stage &stage)
{
    if (stage.backdropTexture.empty())
        return "";
    return stage.backdropName[stage.currentBackdropIndex];
}
double size(const Sprite &sprite)
{
    return sprite.spriteSize;
}
void say(Sprite &sprite, const string &text, SDL_Renderer* renderer, TTF_Font* font)
{
    sprite.saying=true;
    if (sprite.textTexture!=nullptr)
    {
        SDL_DestroyTexture(sprite.textTexture);
        sprite.textTexture=nullptr;
    }
    SDL_Surface* textSurface=TTF_RenderText_Blended(font, text.c_str(), {0, 0, 0, 255});
    sprite.textTexture=SDL_CreateTextureFromSurface(renderer, textSurface);
    sprite.textRect.w=textSurface->w;
    sprite.textRect.h=textSurface->h;
    SDL_FreeSurface(textSurface);
    sprite.bubbleRect.w=sprite.textRect.w+20;
    sprite.bubbleRect.h=sprite.textRect.h+20;
    //the bubbleRect should be slightly be larger than the textRect so the text can fit in the bubble
    sprite.bubbleEnabledTime=-1;
}
void think(Sprite &sprite, const string &text, SDL_Renderer* renderer, TTF_Font* font)
{
    sprite.thinking=true;
    if (sprite.textTexture!=nullptr)
    {
        SDL_DestroyTexture(sprite.textTexture);
        sprite.textTexture=nullptr;
    }
    SDL_Surface* textSurface=TTF_RenderText_Blended(font, text.c_str(), {0, 0, 0, 255});
    sprite.textTexture=SDL_CreateTextureFromSurface(renderer, textSurface);
    sprite.textRect.w=textSurface->w;
    sprite.textRect.h=textSurface->h;
    SDL_FreeSurface(textSurface);
    sprite.bubbleRect.w=sprite.textRect.w+20;
    sprite.bubbleRect.h=sprite.textRect.h+20;
    //the bubbleRect should be slightly be larger than the textRect so the text can fit in the bubble
    sprite.bubbleEnabledTime=-1;
}
void sayForSeconds(Sprite &sprite, const string &text, SDL_Renderer* renderer, TTF_Font* font, double time)
{
    sprite.saying=true;
    if (sprite.textTexture!=nullptr)
    {
        SDL_DestroyTexture(sprite.textTexture);
        sprite.textTexture=nullptr;
    }
    SDL_Surface* textSurface=TTF_RenderText_Blended(font, text.c_str(), {0, 0, 0, 255});
    sprite.textTexture=SDL_CreateTextureFromSurface(renderer, textSurface);
    sprite.textRect.w=textSurface->w;
    sprite.textRect.h=textSurface->h;
    SDL_FreeSurface(textSurface);
    sprite.bubbleRect.w=sprite.textRect.w+20;
    sprite.bubbleRect.h=sprite.textRect.h+20;
    //the bubbleRect should be slightly be larger than the textRect so the text can fit in the bubble
    sprite.bubbleEnabledTime=static_cast<Uint32>(time * 1000)+SDL_GetTicks();
}
void thinkForSeconds(Sprite &sprite, const string &text, SDL_Renderer* renderer, TTF_Font* font, double time)
{
    sprite.thinking=true;
    if (sprite.textTexture!=nullptr)
    {
        SDL_DestroyTexture(sprite.textTexture);
        sprite.textTexture=nullptr;
    }
    SDL_Surface* textSurface=TTF_RenderText_Blended(font, text.c_str(), {0, 0, 0, 255});
    sprite.textTexture=SDL_CreateTextureFromSurface(renderer, textSurface);
    sprite.textRect.w=textSurface->w;
    sprite.textRect.h=textSurface->h;
    SDL_FreeSurface(textSurface);
    sprite.bubbleRect.w=sprite.textRect.w+20;
    sprite.bubbleRect.h=sprite.textRect.h+20;
    //the bubbleRect should be slightly be larger than the textRect so the text can fit in the bubble
    sprite.bubbleEnabledTime=static_cast<Uint32>(time * 1000)+SDL_GetTicks();
}
