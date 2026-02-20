#include "sprite.h"

void setGameArea(int x, int y, int w, int h)
{
    gameArea.x = x;
    gameArea.y = y;
    gameArea.w = w;
    gameArea.h = h;
}

void correctDirRange(Sprite& sprite)
{
    while (sprite.direction > 180)
        sprite.direction -= 360;
    while (sprite.direction < -180)
        sprite.direction += 360;
}

Sprite createSprite(SDL_Texture* costumeTexture, const string& costumeName, int windowWidth, int windowLength)
{
    Sprite sprite;
    sprite.visible = true;
    sprite.spriteCostumes.push_back({costumeName, costumeTexture});
    sprite.currentCostumeIndex = 0;
    SDL_QueryTexture(costumeTexture, nullptr, nullptr,
                     &sprite.costumeWidth, &sprite.costumeHeight);
    sprite.direction = 0;
    sprite.xCenter = windowWidth / 2.0;
    sprite.yCenter = windowLength / 2.0;
    sprite.spriteSize = 100;
    sprite.graphicEffectEnabled = false; // in the default mode no graphic effect is on
    sprite.colorEffect = 0;
    sprite.thinking = false;
    sprite.saying = false;
    sprite.draggable = false;
    sprite.dragging = false;
    return sprite;
}

void drawSprite(SDL_Renderer* renderer, Sprite& sprite,
                SDL_Texture* sayBubbleTexture, SDL_Texture* thinkBubbleTexture)
{
    if (sprite.visible == false)
        return;
    SDL_Texture* currentTexture = sprite.spriteCostumes[sprite.currentCostumeIndex].costumeTexture;
    SDL_Rect spriteRect;
    spriteRect.w = sprite.costumeWidth * sprite.spriteSize / 100;
    spriteRect.h = sprite.costumeHeight * sprite.spriteSize / 100;
    spriteRect.x = sprite.xCenter - (sprite.costumeWidth * sprite.spriteSize / 100) / 2.0;
    spriteRect.y = sprite.yCenter - (sprite.costumeHeight * sprite.spriteSize / 100) / 2.0;
    SDL_RenderCopyEx(renderer, currentTexture, nullptr,
                     &spriteRect, -sprite.direction, nullptr, SDL_FLIP_NONE);
    //In SDL the texture color cant be changed therefore to be able to perform the color effects on the sprite
    // we use a colored transparent rect on the sprite so the illusion of color change is made
    if (sprite.graphicEffectEnabled)
    {
        Uint8 r = static_cast<int>(sprite.colorEffect) % 255;
        Uint8 g = static_cast<int>(sprite.colorEffect + 85) % 255;
        Uint8 b = static_cast<int>(sprite.colorEffect + 170) % 255;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, r, g, b, 100);
        SDL_RenderFillRect(renderer, &spriteRect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    if (SDL_GetTicks() > sprite.bubbleEnabledTime && sprite.bubbleEnabledTime > 0)
    {
        sprite.saying = false;
        sprite.thinking = false;
        sprite.bubbleEnabledTime = 0;
    }
    SDL_Texture* bubble = nullptr;
    if (sprite.saying)
        bubble = sayBubbleTexture;
    if (sprite.thinking)
        bubble = thinkBubbleTexture;
    if (bubble != nullptr)
    {
        SDL_Rect dstBubble;
        dstBubble.w = sprite.bubbleRect.w;
        dstBubble.h = sprite.bubbleRect.h;
        dstBubble.x = sprite.xCenter + sprite.costumeWidth / 2.0;
        dstBubble.y = sprite.yCenter - sprite.costumeHeight / 2.0 - dstBubble.h;
        SDL_Rect dstText;
        dstText.w = sprite.textRect.w;
        dstText.h = sprite.textRect.h;
        dstText.x = sprite.xCenter + sprite.costumeWidth / 2.0 + 10;
        dstText.y = sprite.yCenter - sprite.costumeHeight / 2.0 - dstBubble.h + 10;
        SDL_RenderCopy(renderer, bubble, nullptr, &dstBubble);
        SDL_RenderCopy(renderer, sprite.textTexture, nullptr, &dstText);
    }
}

void setUpStage(Stage& stage)
{
    stage.stageRect.x = gameArea.x;
    stage.stageRect.y = gameArea.y;
    stage.stageRect.w = gameArea.w;
    stage.stageRect.w = gameArea.h;
    stage.currentBackdropIndex = -1; // this means that no backdrop is being selected at first
}

void drawStage(SDL_Renderer* renderer, const Stage& stage)
{
    if (stage.currentBackdropIndex == -1)
        return;
    SDL_RenderCopy(renderer, stage.backdropTexture[stage.currentBackdropIndex],
                   nullptr, &stage.stageRect);
}

void spriteDrag(Sprite& sprite, SDL_Event& event)
{
    if (sprite.draggable == false)
        return;
    int xMouse;
    int yMouse;
    SDL_GetMouseState(&xMouse, &yMouse);
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
    {
        if (xMouse >= sprite.xCenter - sprite.costumeWidth / 2.0 &&
            xMouse <= sprite.xCenter + sprite.costumeWidth / 2.0 &&
            yMouse >= sprite.yCenter - sprite.costumeHeight / 2.0 &&
            yMouse <= sprite.yCenter + sprite.costumeHeight / 2.0)
        {
            sprite.dragging = true;
        }
    }
    else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
    {
        sprite.dragging = false;
    }
    if (sprite.dragging)
    {
        sprite.xCenter = xMouse;
        sprite.yCenter = yMouse;
    }
}

void mute(Sprite& sprite, const string& soundName)
{
    auto soundIt = sprite.spriteSounds.find(soundName);
    if (soundIt == sprite.spriteSounds.end())
        return;
    Mix_VolumeChunk(sprite.spriteSounds[soundName], 0);
}

void unmute(Sprite& sprite, const string& soundName)
{
    auto soundIt = sprite.spriteSounds.find(soundName);
    if (soundIt == sprite.spriteSounds.end())
        return;
    int volume = sprite.soundVolumes[soundName];
    Mix_VolumeChunk(sprite.spriteSounds[soundName], volume);
}
