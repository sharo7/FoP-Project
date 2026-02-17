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
void clearGraphicEffects(Sprite &sprite);
void setColorEffectTo(Sprite &sprite, double value);
void changeColorEffectBy(Sprite &sprite, double value);
void addBackdrop(Stage &stage, SDL_Texture* backdropTexture, const string& backdropName);
void switchBackdropTo(Stage &stage, const string backdropName);
void nextBackdrop(Stage &stage);
int costumeNumber(const Sprite &sprite);
string costumeName(const Sprite &sprite);
int backdropNumber(const Stage &stage);
string backdropName(const Stage &stage);
double size(const Sprite &sprite);
#endif //LOOKS_COMMANDS_H