#ifndef SPRITE_H
#define SPRITE_H
#include <bits/stdc++.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
using namespace std;
struct Stage
{
    SDL_Rect stageRect; //this is the rectangle that the sprite lives on
    vector<string> backdropName;
    vector<SDL_Texture*> backdropTexture;
    int currentBackdropIndex;
};
struct Costume //this is done instead of using a map because the order and number of the costume is important
{
    string costumeName;
    SDL_Texture* costumeTexture;
};
struct Sprite
{
    vector<Costume> spriteCostumes;
    int currentCostumeIndex;
    int costumeWidth, costumeHeight;
    double xCenter, yCenter;//sprite's center coordinates on the sdl default coordination system
    double direction;//the angle in degrees
    //direction 0 is pointing right and rotation direction is anticlockwise
    bool visible;
    double spriteSize;// this is the scale of the sprite in percentages
    bool graphicEffectEnabled;
    double colorEffect;//this number decides the change of color on the sprite
    bool thinking;
    bool saying;
    map<string, Mix_Chunk*> spriteSounds;

};

void correctDirRange(Sprite& sprite);//valid range for the angle is -180 to 180 degrees
Sprite createSprite(SDL_Texture* texture, int windowWidth, int windowLength);
void drawSprite(SDL_Renderer* renderer, const Sprite &sprite);
void setUpStage(Stage &stage, int x, int y, int w, int h);//this function should be called at the first of the
//program to save the information of the stage that the sprite performs on
void drawStage(SDL_Renderer* renderer, const Stage &stage);
#endif //SPRITE_H