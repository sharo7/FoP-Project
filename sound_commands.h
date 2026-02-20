#ifndef SOUND_COMMANDS_H
#define SOUND_COMMANDS_H
#include "sprite.h"

//Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize) is for initializing the sound
//and should be added to the main

void addSound(Sprite &sprite, const string &soundFile, const string &SoundName);

void startSound(Sprite &sprite, const string &soundName);

void playSoundUntilDone(Sprite &sprite, const string &soundName); //the program will stop until the sound is played

void stopAllSounds(Sprite &sprite);

void setVolumeTo(Sprite &sprite, const string &soundName, int volume);

void changeVolumeBy(Sprite &sprite, const string &soundName, int volume);

void freeAllSounds(Sprite &sprite); //this function should be called at the end of the program for every sprite that

//has used sound to free the memory from all of them
void soundInitializer(); // this should be added at the first of the program to initialize the sound

#endif //SOUND_COMMANDS_H
