#ifndef SOUND_COMMANDS_H
#define SOUND_COMMANDS_H
#include "sprite.h"

//Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize) is for initializing the sound and
// should be added to the main

void addSound(Sprite &sprite, const string &soundFile, const string &SoundName);

void startSound(Sprite &sprite, const string &soundName);

void playSoundUntilDone(Sprite &sprite, const string &soundName); //the program will stop until the sound is played

void stopAllSounds(Sprite &sprite);

void setVolumeTo(Sprite &sprite, const string &soundName, int volume);

void changeVolumeBy(Sprite &sprite, const string &soundName, int volume);

#endif //SOUND_COMMANDS_H
