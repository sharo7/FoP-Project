#include "sprite.h"

void correctDirRange(Sprite &sprite) {
    while (sprite.direction > 180)
        sprite.direction -= 360;
    while (sprite.direction <= -180)
        sprite.direction += 360;
}

Sprite createSprite(SDL_Texture *texture, int windowWidth, int windowLength) {
    Sprite sprite;
    sprite.visible = true;
    sprite.spriteTexture = texture;
    SDL_QueryTexture(sprite.spriteTexture, nullptr, nullptr,
                     &sprite.spriteWidth, &sprite.spriteHeight);
    sprite.direction = 0;
    sprite.xCenter = windowWidth / 2.0;
    sprite.yCenter = windowLength / 2.0;
    return sprite;
}
