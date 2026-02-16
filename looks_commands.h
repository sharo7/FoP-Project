#ifndef LOOKS_COMMANDS_H
#define LOOKS_COMMANDS_H
#include "sprite.h"
void show(Sprite &sprite);
void hide(Sprite &sprite);
void addCostume(Sprite &sprite, SDL_Texture* costumeTexture, const string &costumeName);
void nextCostume(Sprite &sprite);
void switchCostumeTo(Sprite &sprite, const string &costumeName);
void setSizeTo(Sprite &sprite, double scalePercentage);
void changeSizeBy(Sprite &sprite, double scalePercentage);
#endif //LOOKS_COMMANDS_H