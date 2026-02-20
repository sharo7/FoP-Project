#include "sound_commands.h"

void addSound(Sprite &sprite, const string &soundFile, const string &soundName) {
    sprite.spriteSounds[soundName] = Mix_LoadWAV(soundFile.c_str());
    sprite.soundVolumes[soundName] = MIX_MAX_VOLUME; //the default volume
}

void startSound(Sprite &sprite, const string &soundName) {
    auto soundIt = sprite.spriteSounds.find(soundName);
    if (soundIt == sprite.spriteSounds.end())
        return;
    Mix_PlayChannel(-1, sprite.spriteSounds[soundName], 0);
}

void playSoundUntilDone(Sprite &sprite, const string &soundName) {
    auto soundIt = sprite.spriteSounds.find(soundName);
    if (soundIt == sprite.spriteSounds.end())
        return;
    while (Mix_Playing(Mix_PlayChannel(-1, sprite.spriteSounds[soundName], 0))) {
        //this loop will not end until the sound ends so the program will not continue up until then
    }
}

void stopAllSounds(Sprite &sprite) {
    Mix_HaltChannel(-1); //this stops all the playing sounds
}

//the volume range is 0 to 100
void setVolumeTo(Sprite &sprite, const string &soundName, int volume) {
    volume *= MIX_MAX_VOLUME / 100;
    //the volume range for the function that is being used is 0 to 120 so this line converts
    // the volume to the desired range
    auto soundIt = sprite.spriteSounds.find(soundName);
    if (soundIt == sprite.spriteSounds.end())
        return;
    Mix_VolumeChunk(sprite.spriteSounds[soundName], volume);
    sprite.soundVolumes[soundName] = volume;
}

void changeVolumeBy(Sprite &sprite, const string &soundName, int volume) {
    auto soundIt = sprite.spriteSounds.find(soundName);
    if (soundIt == sprite.spriteSounds.end())
        return;
    int currentVolume = Mix_VolumeChunk(sprite.spriteSounds[soundName], -1);
    volume *= MIX_MAX_VOLUME / 100;
    volume += currentVolume;
    if (volume < 0)
        volume = 0;
    if (volume > MIX_MAX_VOLUME)
        volume = MIX_MAX_VOLUME;
    Mix_VolumeChunk(sprite.spriteSounds[soundName], volume);
    sprite.soundVolumes[soundName] = volume;
}

void freeAllSounds(Sprite &sprite) {
    for (auto &sound: sprite.spriteSounds)
        Mix_FreeChunk(sound.second);
    sprite.spriteSounds.clear();
}

void soundInitializer() {
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
}
