/*
 * audio_wasapi.c - Windows Audio Session API (WASAPI) Audio Playback
 *
 * Low-latency audio playback for Windows using WASAPI.
 * Attempts exclusive mode first, falls back to shared mode.
 */

#ifdef _WIN32

#include "../include/rootstream.h"
#include <stdio.h>
#include <string.h>

/* WASAPI headers */
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>

/* Reference time units (100-nanosecond intervals) */
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

/* Audio configuration matching Linux ALSA settings */
#define AUDIO_SAMPLE_RATE    48000
#define AUDIO_CHANNELS       2
#define AUDIO_BITS_PER_SAMPLE 16
#define AUDIO_FRAME_SIZE     960   /* 20ms at 48kHz */

/* WASAPI context stored in rootstream_ctx_t */
typedef struct {
    IMMDeviceEnumerator *enumerator;
    IMMDevice *device;
    IAudioClient *audio_client;
    IAudioRenderClient *render_client;
    WAVEFORMATEX *wave_format;
    UINT32 buffer_frames;
    HANDLE event;
    bool exclusive_mode;
    bool initialized;
    bool started;
} wasapi_ctx_t;

/* GUIDs */
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c,
            0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xa95664d2, 0x9614, 0x4f35,
            0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);
DEFINE_GUID(IID_IAudioClient, 0x1cb9ad4c, 0xdbfa, 0x4c32,
            0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2);
DEFINE_GUID(IID_IAudioRenderClient, 0xf294acfc, 0x3146, 0x4483,
            0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2);

/* Forward declarations */
static int wasapi_init_exclusive(wasapi_ctx_t *ctx);
static int wasapi_init_shared(wasapi_ctx_t *ctx);

/* ============================================================================
 * Initialization
 * ============================================================================ */

int audio_playback_init(rootstream_ctx_t *ctx) {
    HRESULT hr;
    wasapi_ctx_t *wasapi;

    /* Allocate WASAPI context */
    wasapi = (wasapi_ctx_t *)calloc(1, sizeof(wasapi_ctx_t));
    if (!wasapi) {
        fprintf(stderr, "WASAPI: Failed to allocate context\n");
        return -1;
    }

    /* Store in rootstream context (use audio_playback field) */
    ctx->audio_playback.backend_ctx = wasapi;

    /* Initialize COM if needed */
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        fprintf(stderr, "WASAPI: COM initialization failed: 0x%08lx\n", hr);
        free(wasapi);
        return -1;
    }

    /* Create device enumerator */
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                          &IID_IMMDeviceEnumerator, (void **)&wasapi->enumerator);
    if (FAILED(hr)) {
        fprintf(stderr, "WASAPI: Failed to create device enumerator: 0x%08lx\n", hr);
        free(wasapi);
        return -1;
    }

    /* Get default audio output device */
    hr = wasapi->enumerator->lpVtbl->GetDefaultAudioEndpoint(
        wasapi->enumerator, eRender, eConsole, &wasapi->device);
    if (FAILED(hr)) {
        fprintf(stderr, "WASAPI: Failed to get default audio device: 0x%08lx\n", hr);
        wasapi->enumerator->lpVtbl->Release(wasapi->enumerator);
        free(wasapi);
        return -1;
    }

    /* Activate audio client */
    hr = wasapi->device->lpVtbl->Activate(
        wasapi->device, &IID_IAudioClient, CLSCTX_ALL, NULL,
        (void **)&wasapi->audio_client);
    if (FAILED(hr)) {
        fprintf(stderr, "WASAPI: Failed to activate audio client: 0x%08lx\n", hr);
        wasapi->device->lpVtbl->Release(wasapi->device);
        wasapi->enumerator->lpVtbl->Release(wasapi->enumerator);
        free(wasapi);
        return -1;
    }

    /* Create event for buffer notifications */
    wasapi->event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!wasapi->event) {
        fprintf(stderr, "WASAPI: Failed to create event\n");
        wasapi->audio_client->lpVtbl->Release(wasapi->audio_client);
        wasapi->device->lpVtbl->Release(wasapi->device);
        wasapi->enumerator->lpVtbl->Release(wasapi->enumerator);
        free(wasapi);
        return -1;
    }

    /* Try exclusive mode first for lowest latency */
    if (wasapi_init_exclusive(wasapi) == 0) {
        wasapi->exclusive_mode = true;
        printf("WASAPI: Initialized in exclusive mode (lowest latency)\n");
    } else {
        /* Fall back to shared mode */
        if (wasapi_init_shared(wasapi) != 0) {
            fprintf(stderr, "WASAPI: Failed to initialize in any mode\n");
            CloseHandle(wasapi->event);
            wasapi->audio_client->lpVtbl->Release(wasapi->audio_client);
            wasapi->device->lpVtbl->Release(wasapi->device);
            wasapi->enumerator->lpVtbl->Release(wasapi->enumerator);
            free(wasapi);
            return -1;
        }
        wasapi->exclusive_mode = false;
        printf("WASAPI: Initialized in shared mode\n");
    }

    /* Get render client */
    hr = wasapi->audio_client->lpVtbl->GetService(
        wasapi->audio_client, &IID_IAudioRenderClient,
        (void **)&wasapi->render_client);
    if (FAILED(hr)) {
        fprintf(stderr, "WASAPI: Failed to get render client: 0x%08lx\n", hr);
        if (wasapi->wave_format) CoTaskMemFree(wasapi->wave_format);
        CloseHandle(wasapi->event);
        wasapi->audio_client->lpVtbl->Release(wasapi->audio_client);
        wasapi->device->lpVtbl->Release(wasapi->device);
        wasapi->enumerator->lpVtbl->Release(wasapi->enumerator);
        free(wasapi);
        return -1;
    }

    /* Get buffer size */
    hr = wasapi->audio_client->lpVtbl->GetBufferSize(
        wasapi->audio_client, &wasapi->buffer_frames);
    if (FAILED(hr)) {
        fprintf(stderr, "WASAPI: Failed to get buffer size: 0x%08lx\n", hr);
        wasapi->render_client->lpVtbl->Release(wasapi->render_client);
        if (wasapi->wave_format) CoTaskMemFree(wasapi->wave_format);
        CloseHandle(wasapi->event);
        wasapi->audio_client->lpVtbl->Release(wasapi->audio_client);
        wasapi->device->lpVtbl->Release(wasapi->device);
        wasapi->enumerator->lpVtbl->Release(wasapi->enumerator);
        free(wasapi);
        return -1;
    }

    printf("WASAPI: Buffer size: %u frames (%.1f ms)\n",
           wasapi->buffer_frames,
           (float)wasapi->buffer_frames * 1000.0f / AUDIO_SAMPLE_RATE);

    wasapi->initialized = true;
    wasapi->started = false;

    return 0;
}

static int wasapi_init_exclusive(wasapi_ctx_t *ctx) {
    HRESULT hr;
    WAVEFORMATEX format;
    REFERENCE_TIME buffer_duration;

    /* Set up format for exclusive mode */
    memset(&format, 0, sizeof(format));
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = AUDIO_CHANNELS;
    format.nSamplesPerSec = AUDIO_SAMPLE_RATE;
    format.wBitsPerSample = AUDIO_BITS_PER_SAMPLE;
    format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    format.cbSize = 0;

    /* Request 10ms buffer for low latency */
    buffer_duration = 10 * REFTIMES_PER_MILLISEC;

    /* Initialize in exclusive mode */
    hr = ctx->audio_client->lpVtbl->Initialize(
        ctx->audio_client,
        AUDCLNT_SHAREMODE_EXCLUSIVE,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        buffer_duration,
        buffer_duration,
        &format,
        NULL);

    if (FAILED(hr)) {
        return -1;
    }

    /* Set event handle */
    hr = ctx->audio_client->lpVtbl->SetEventHandle(ctx->audio_client, ctx->event);
    if (FAILED(hr)) {
        return -1;
    }

    /* Store format (copy) */
    ctx->wave_format = (WAVEFORMATEX *)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
    if (ctx->wave_format) {
        memcpy(ctx->wave_format, &format, sizeof(WAVEFORMATEX));
    }

    return 0;
}

static int wasapi_init_shared(wasapi_ctx_t *ctx) {
    HRESULT hr;
    WAVEFORMATEX *device_format = NULL;
    REFERENCE_TIME buffer_duration;

    /* Get device's mix format */
    hr = ctx->audio_client->lpVtbl->GetMixFormat(ctx->audio_client, &device_format);
    if (FAILED(hr)) {
        return -1;
    }

    /* Request 20ms buffer */
    buffer_duration = 20 * REFTIMES_PER_MILLISEC;

    /* Initialize in shared mode */
    hr = ctx->audio_client->lpVtbl->Initialize(
        ctx->audio_client,
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
        AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
        buffer_duration,
        0,  /* periodicity must be 0 for shared mode */
        device_format,
        NULL);

    if (FAILED(hr)) {
        CoTaskMemFree(device_format);
        return -1;
    }

    /* Set event handle */
    hr = ctx->audio_client->lpVtbl->SetEventHandle(ctx->audio_client, ctx->event);
    if (FAILED(hr)) {
        CoTaskMemFree(device_format);
        return -1;
    }

    ctx->wave_format = device_format;
    return 0;
}

/* ============================================================================
 * Playback
 * ============================================================================ */

int audio_playback_write(rootstream_ctx_t *ctx, int16_t *samples, size_t num_samples) {
    wasapi_ctx_t *wasapi = (wasapi_ctx_t *)ctx->audio_playback.backend_ctx;
    HRESULT hr;
    UINT32 padding;
    UINT32 available;
    BYTE *buffer;

    if (!wasapi || !wasapi->initialized) {
        return -1;
    }

    /* Start playback on first write */
    if (!wasapi->started) {
        hr = wasapi->audio_client->lpVtbl->Start(wasapi->audio_client);
        if (FAILED(hr)) {
            fprintf(stderr, "WASAPI: Failed to start playback: 0x%08lx\n", hr);
            return -1;
        }
        wasapi->started = true;
    }

    /* Get current padding (how much is already buffered) */
    hr = wasapi->audio_client->lpVtbl->GetCurrentPadding(wasapi->audio_client, &padding);
    if (FAILED(hr)) {
        fprintf(stderr, "WASAPI: Failed to get padding: 0x%08lx\n", hr);
        return -1;
    }

    /* Calculate available space */
    available = wasapi->buffer_frames - padding;
    if (num_samples > available) {
        /* Buffer full, wait for space */
        WaitForSingleObject(wasapi->event, 10);

        /* Re-check padding */
        hr = wasapi->audio_client->lpVtbl->GetCurrentPadding(wasapi->audio_client, &padding);
        if (FAILED(hr)) {
            return -1;
        }
        available = wasapi->buffer_frames - padding;
    }

    /* Limit to available space */
    UINT32 frames_to_write = (UINT32)num_samples;
    if (frames_to_write > available) {
        frames_to_write = available;
    }

    if (frames_to_write == 0) {
        return 0;
    }

    /* Get buffer */
    hr = wasapi->render_client->lpVtbl->GetBuffer(
        wasapi->render_client, frames_to_write, &buffer);
    if (FAILED(hr)) {
        fprintf(stderr, "WASAPI: Failed to get buffer: 0x%08lx\n", hr);
        return -1;
    }

    /* Copy samples (16-bit stereo) */
    memcpy(buffer, samples, frames_to_write * AUDIO_CHANNELS * sizeof(int16_t));

    /* Release buffer */
    hr = wasapi->render_client->lpVtbl->ReleaseBuffer(
        wasapi->render_client, frames_to_write, 0);
    if (FAILED(hr)) {
        fprintf(stderr, "WASAPI: Failed to release buffer: 0x%08lx\n", hr);
        return -1;
    }

    return (int)frames_to_write;
}

/* ============================================================================
 * Cleanup
 * ============================================================================ */

void audio_playback_cleanup(rootstream_ctx_t *ctx) {
    wasapi_ctx_t *wasapi = (wasapi_ctx_t *)ctx->audio_playback.backend_ctx;

    if (!wasapi) {
        return;
    }

    /* Stop playback */
    if (wasapi->started && wasapi->audio_client) {
        wasapi->audio_client->lpVtbl->Stop(wasapi->audio_client);
    }

    /* Release resources */
    if (wasapi->render_client) {
        wasapi->render_client->lpVtbl->Release(wasapi->render_client);
    }
    if (wasapi->wave_format) {
        CoTaskMemFree(wasapi->wave_format);
    }
    if (wasapi->event) {
        CloseHandle(wasapi->event);
    }
    if (wasapi->audio_client) {
        wasapi->audio_client->lpVtbl->Release(wasapi->audio_client);
    }
    if (wasapi->device) {
        wasapi->device->lpVtbl->Release(wasapi->device);
    }
    if (wasapi->enumerator) {
        wasapi->enumerator->lpVtbl->Release(wasapi->enumerator);
    }

    free(wasapi);
    ctx->audio_playback.backend_ctx = NULL;

    printf("WASAPI: Cleanup complete\n");
}

#endif /* _WIN32 */
