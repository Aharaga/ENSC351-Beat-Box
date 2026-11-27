#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#include "audio.h"

#define SAMPLE_RATE 44100
#define CHANNELS 2
#define BUFFER_SIZE 4096
#define WAV_HEADER_SIZE 44


// ALSA & File handles
static snd_pcm_t *pcm_handle = NULL;
static FILE *fileA = NULL;
static FILE *fileB = NULL;

// Thread and control flags
static pthread_t audio_thread;
static volatile bool running = false;
static pthread_mutex_t audio_mutex = PTHREAD_MUTEX_INITIALIZER;

// State variables
static float volA = 1.0f; // 100% volume
static float volB = 1.0f; // 100% volume
static float crossfade = 0.5f; // 50/50 mix

// Helper function to clamp 32-bit sample sum into 16-bit signed PCM range
static inline int16_t clamp16(int32_t sample)
{
    if(sample > 32767) return 32767;
    if(sample < -32768) return -32768;
    return (int16_t)sample;
}


// Background thread
static void *audioThreadRoutine(void *arg)
{
    (void)arg;
    
    int16_t buffA[BUFFER_SIZE];
    int16_t buffB[BUFFER_SIZE];
    int16_t outBuff[BUFFER_SIZE];

    while(running)
    {
        // Read samples from track A
        size_t samplesReadA = fread(buffA, sizeof(int16_t), BUFFER_SIZE, fileA);
        if(samplesReadA < BUFFER_SIZE)
        {
            fseek(fileA, WAV_HEADER_SIZE, SEEK_SET); // Auto loop track A
            fread(buffA + samplesReadA, sizeof(int16_t), BUFFER_SIZE - samplesReadA, fileA);
        }
        // Read samples from track B
        size_t samplesReadB = fread(buffB, sizeof(int16_t), BUFFER_SIZE, fileB);
        if(samplesReadB < BUFFER_SIZE)
        {
            fseek(fileB, WAV_HEADER_SIZE, SEEK_SET); // Auto loop track A
            fread(buffB + samplesReadB, sizeof(int16_t), BUFFER_SIZE - samplesReadB, fileB);
        }

        // Thread safe copy of mixing parameters
        pthread_mutex_lock(&audio_mutex);
        float currentVolA = volA;
        float currentVolB = volB;
        float currentCrossfade = crossfade;
        pthread_mutex_unlock(&audio_mutex);

        // Calculate channel multipliers using crossfade position
        // Crossfade = 0 trackA = 100% trackB = 0% | Crossfade = 1.0 trackA = 0% trackB = 100%
        float gainA = currentVolA * (1.0f - currentCrossfade);
        float gainB = currentVolB * currentCrossfade;

        // Mix PCM samples
        for(int i = 0; i < BUFFER_SIZE; i++)
        {
            int32_t sampleA = (int32_t)(buffA[i]*gainA);
            int32_t sampleB = (int32_t)(buffB[i]*gainB);
            outBuff[i] = clamp16(sampleA + sampleB);
        }

        // Write mixed frams to ALSA PCM hardware
        snd_pcm_uframes_t frames = BUFFER_SIZE / CHANNELS;
        snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, outBuff, frames);

        if(written < 0)
        {
            written = snd_pcm_recover(pcm_handle, written, 0); // recover from underruns
        }

    }
    return NULL;
}

bool Audio_init(const char *trackA, const char *trackB)
{
    fileA = fopen(trackA, "rb");
    fileB = fopen(trackB, "rb");

    if(!fileA || !fileB)
    {
        perror("AUDIO INIT ERROR: Failed to open one or both .wav files");
        if(fileA) fclose(fileA);
        if(fileB) fclose(fileB);
        return false;
    }

    // Skip WAV headers
    fseek(fileA, WAV_HEADER_SIZE, SEEK_SET);
    fseek(fileB, WAV_HEADER_SIZE, SEEK_SET);

    // Open ALSA playback handle
    int err = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK,0);
    if (err < 0)
    {
        fprintf(stderr, "AUDIO INIT ERROR: ALSA open error %s\n", snd_strerror(err));
        fclose(fileA);
        fclose(fileB);
        return false;
    }

    // Configure for 44.1 kHz, 16-bit Stereo
    err = snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, CHANNELS, SAMPLE_RATE, 1, 500000);
    
    if(err < 0)
    {
        fprintf(stderr, "AUDIO INIT ERROR: ALSA parameter error: %s\n", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        fclose(fileA);
        fclose(fileB);
        return false;
    }

    // Start background thread
    running = true;
    if(pthread_create(&audio_thread, NULL, audioThreadRoutine, NULL) != 0)
    {
        perror("AUDIO INIT ERROR: Failed to create audio thread\n");
        running = false;
        snd_pcm_close(pcm_handle);
        fclose(fileA);
        fclose(fileB);
        return false;
    }
    printf("Audio Engine Started Successfully.\n");
    return true;
}

void Audio_cleanup(void)
{
    if(!running) return;

    running = false;
    pthread_join(audio_thread, NULL);

    if(pcm_handle)
    {
        snd_pcm_drain(pcm_handle);
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
    }

    if (fileA) fclose(fileA);
    if (fileB) fclose(fileB);

    printf("Audio Engine Cleaned Up.\n");

}



void Audio_setVolumeA(float vol)
{
    pthread_mutex_lock(&audio_mutex);
    volA = (vol < 0.0f) ? 0.0f : (vol > 1.0f) ? 1.0f : vol;
    pthread_mutex_unlock(&audio_mutex);
}

void Audio_setVolumeB(float vol)
{
    pthread_mutex_lock(&audio_mutex);
    volB = (vol < 0.0f) ? 0.0f : (vol > 1.0f) ? 1.0f : vol;
    pthread_mutex_unlock(&audio_mutex);
}

float Audio_getVolumeA(void) { return volA; }
float Audio_getVolumeB(void) { return volB; }


// CROSSFADE FUNCTIONS
void Audio_setCrossfade(float mix)
{
    pthread_mutex_lock(&audio_mutex);
    crossfade = (mix < 0.0f) ? 0.0f : (mix > 1.0f) ? 1.0f : mix;
    pthread_mutex_unlock(&audio_mutex);
}

float Audio_getCrossfade(void) { return crossfade; }



// DEBUG PLAY 1 TRACK
int playSingleWAV(const char *filename)
{
    FILE *file = fopen(filename, "rb");

    // Validate file
    if(!file)
    {
        perror("ERROR: Failed to open WAV file");
        return -1;
    }

    //Skip 44-byte WAV header to get raw PCM samples
    fseek(file, 44, SEEK_SET);

    snd_pcm_t *handle;
    int err;

    // Attempt to open default ALSA audio device
    if((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)))
    {
        fprintf(stderr, "ALSA open error: %s\n", snd_strerror(err));
        fclose(file);
        return -1;
    }

    //Configure parameters: 16-bit stereo PCM @ 44.1 kHz
    if((err = snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, CHANNELS, SAMPLE_RATE, 1, 500000)) < 0)
    {
        fprintf(stderr, "ALSA parameter error: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        fclose(file);
        return -1;
    }

    short buffer[BUFFER_SIZE];
    size_t bytesRead;

    printf("Now Playing %s...\n", filename);

    // Read raw PCM audio and send to ALSA buffer
    while((bytesRead = fread(buffer, sizeof(short), BUFFER_SIZE, file)) > 0)
    {
        // Calculate number of frames
        snd_pcm_uframes_t frames = bytesRead / CHANNELS; 

        snd_pcm_sframes_t written = snd_pcm_writei(handle, buffer, frames);
        
        // Recover if potential buffer overrun
        if(written < 0)
        {
            written = snd_pcm_recover(handle, written, 0);
        }
        // Check if recovery was unsuccesful
        if(written < 0)
        {
            fprintf(stderr, "ALSA write error: %s\n", snd_strerror(written));
            break;
        }
    }

    // Cleanup
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    fclose(file);

    printf("Playback finished.\n");
    return 0;
}