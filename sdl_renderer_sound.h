#include "SDL2/SDL_audio.h"
#include <SDL2/SDL.h>
#include <stdio.h>

#include "sounds.h"
#include "build/sounds/dzin.h"
#include "build/sounds/bump.h"
#include "build/sounds/spoon.h"
#include "build/sounds/spoon2.h"
#include "build/sounds/click.h"

const int SAMPLE_RATE = 8000;
const int BUFFER_SIZE = 512;

SDL_AudioDeviceID deviceId;

void renderer_play_sound(Sound sound) {
    SDL_ClearQueuedAudio(deviceId);
    SDL_QueueAudio(deviceId, sound.start, sound.length);
}

int renderer_init_sound() {
    SDL_AudioSpec spec = {
        .format = AUDIO_U8,
        .channels = 1,
        .freq = SAMPLE_RATE,
        .samples = BUFFER_SIZE,
        .callback = NULL,
    };

    deviceId = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);

    SDL_PauseAudioDevice(deviceId, 0);
    return 0;
}
