#ifndef SPRITE_H
#define SPRITE_H
#include <bits/stdc++.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

using namespace std;

extern SDL_Rect gameArea; // this is the area where the sprite can move in

void setGameArea(int x, int y, int w, int h); // this function should first be called in main to get the needed

//values of the game area
struct Stage {
    SDL_Rect stageRect; //this is the rectangle that the sprite lives on
    vector<string> backdropName;
    vector<SDL_Texture *> backdropTexture;
    int currentBackdropIndex;
};

//this is done instead of using a map because the order and number of the costume is important
struct Costume {
    string costumeName;
    SDL_Texture *costumeTexture;
};

struct Sprite {
    vector<Costume> spriteCostumes;
    int currentCostumeIndex;
    int costumeWidth, costumeHeight;
    double xCenter, yCenter; //sprite's center coordinates on the sdl default coordination system
    double direction; //the angle in degrees
    //direction 0 is pointing right and rotation direction is anticlockwise
    bool visible;
    double spriteSize; // this is the scale of the sprite in percentages
    bool graphicEffectEnabled;
    double colorEffect; //this number decides the change of color on the sprite
    bool thinking;
    bool saying;
    SDL_Texture *textTexture;
    SDL_Rect textRect;
    SDL_Rect bubbleRect;
    Uint32 bubbleEnabledTime; //-1 means it is on forever
    bool draggable;
    bool dragging;
    map<string, Mix_Chunk *> spriteSounds;
    map<string, int> soundVolumes;
};

void correctDirRange(Sprite &sprite); //valid range for the angle is -180 to 180 degrees

Sprite createSprite(SDL_Texture *costumeTexture, const string &costumeName, int windowWidth, int windowLength);

void drawSprite(SDL_Renderer *renderer, Sprite &sprite, SDL_Texture *sayBubbleTexture, SDL_Texture *thinkBubbleTexture);

void setUpStage(Stage &stage); //this function should be called at the first of the

//program to save the information of the stage that the sprite performs on
void drawStage(SDL_Renderer *renderer, const Stage &stage);

void spriteDrag(Sprite &sprite, const SDL_Event &event);

// the two following functions are used to mute or unmute a playing sound
void mute(Sprite &sprite, const string &soundName);

void unmute(Sprite &sprite, const string &soundName);

#endif //SPRITE_H
