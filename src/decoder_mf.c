/*
 * decoder_mf.c - Media Foundation Video Decoder for Windows
 *
 * Hardware-accelerated H.264/H.265 decoding using Media Foundation
 * with DXVA2 (DirectX Video Acceleration).
 */

#ifdef _WIN32

#include "../include/rootstream.h"
#include <stdio.h>
#include <string.h>

/* Media Foundation headers */
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

/* Decoder context */
typedef struct {
    IMFTransform *decoder;
    IMFMediaType *input_type;
    IMFMediaType *output_type;
    ID3D11Device *d3d_device;
    ID3D11DeviceContext *d3d_context;
    IMFDXGIDeviceManager *dxgi_manager;
    UINT dxgi_reset_token;

    codec_type_t codec;
    int width;
    int height;
    bool initialized;
    bool mf_started;

    /* Output frame buffer */
    uint8_t *frame_buffer;
    size_t frame_buffer_size;
} mf_decoder_ctx_t;

/* GUIDs for Media Foundation */
static const GUID MF_MT_MAJOR_TYPE_Video = {0x73646976, 0x0000, 0x0010,
    {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID MFMediaType_Video = {0x73646976, 0x0000, 0x0010,
    {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID MFVideoFormat_H264 = {0x34363248, 0x0000, 0x0010,
    {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID MFVideoFormat_HEVC = {0x43564548, 0x0000, 0x0010,
    {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID MFVideoFormat_NV12 = {0x3231564E, 0x0000, 0x0010,
    {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

/* Forward declarations */
static int mf_create_decoder(mf_decoder_ctx_t *ctx);
static int mf_configure_decoder(mf_decoder_ctx_t *ctx);
static int mf_init_d3d11(mf_decoder_ctx_t *ctx);
static void mf_release_resources(mf_decoder_ctx_t *ctx);

/* ============================================================================
 * Initialization
 * ============================================================================ */

int rootstream_decoder_init(rootstream_ctx_t *ctx) {
    HRESULT hr;
    mf_decoder_ctx_t *mf;

    /* Allocate decoder context */
    mf = (mf_decoder_ctx_t *)calloc(1, sizeof(mf_decoder_ctx_t));
    if (!mf) {
        fprintf(stderr, "MF Decoder: Failed to allocate context\n");
        return -1;
    }

    /* Store in rootstream context */
    ctx->decoder.backend_ctx = mf;
    mf->codec = ctx->encoder.codec;  /* Use same codec as encoder setting */
    mf->width = ctx->display.width;
    mf->height = ctx->display.height;

    /* Start Media Foundation */
    hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr)) {
        fprintf(stderr, "MF Decoder: MFStartup failed: 0x%08lx\n", hr);
        free(mf);
        return -1;
    }
    mf->mf_started = true;

    /* Initialize D3D11 for hardware acceleration */
    if (mf_init_d3d11(mf) != 0) {
        fprintf(stderr, "MF Decoder: D3D11 initialization failed, using software decode\n");
        /* Continue without D3D11 - software decoding */
    }

    /* Create and configure decoder */
    if (mf_create_decoder(mf) != 0) {
        fprintf(stderr, "MF Decoder: Failed to create decoder\n");
        mf_release_resources(mf);
        free(mf);
        ctx->decoder.backend_ctx = NULL;
        return -1;
    }

    if (mf_configure_decoder(mf) != 0) {
        fprintf(stderr, "MF Decoder: Failed to configure decoder\n");
        mf_release_resources(mf);
        free(mf);
        ctx->decoder.backend_ctx = NULL;
        return -1;
    }

    /* Allocate output frame buffer (NV12: width * height * 1.5) */
    mf->frame_buffer_size = mf->width * mf->height * 3 / 2;
    mf->frame_buffer = (uint8_t *)malloc(mf->frame_buffer_size);
    if (!mf->frame_buffer) {
        fprintf(stderr, "MF Decoder: Failed to allocate frame buffer\n");
        mf_release_resources(mf);
        free(mf);
        ctx->decoder.backend_ctx = NULL;
        return -1;
    }

    mf->initialized = true;
    printf("MF Decoder: Initialized (%s, %dx%d, %s)\n",
           mf->codec == CODEC_H265 ? "H.265" : "H.264",
           mf->width, mf->height,
           mf->d3d_device ? "hardware" : "software");

    return 0;
}

static int mf_init_d3d11(mf_decoder_ctx_t *ctx) {
    HRESULT hr;
    D3D_FEATURE_LEVEL feature_level;
    UINT flags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT;

#ifdef DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    /* Create D3D11 device */
    hr = D3D11CreateDevice(
        NULL,                    /* Default adapter */
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        flags,
        NULL, 0,                 /* Default feature level */
        D3D11_SDK_VERSION,
        &ctx->d3d_device,
        &feature_level,
        &ctx->d3d_context);

    if (FAILED(hr)) {
        fprintf(stderr, "MF Decoder: D3D11CreateDevice failed: 0x%08lx\n", hr);
        return -1;
    }

    /* Create DXGI device manager */
    hr = MFCreateDXGIDeviceManager(&ctx->dxgi_reset_token, &ctx->dxgi_manager);
    if (FAILED(hr)) {
        fprintf(stderr, "MF Decoder: MFCreateDXGIDeviceManager failed: 0x%08lx\n", hr);
        ctx->d3d_device->lpVtbl->Release(ctx->d3d_device);
        ctx->d3d_context->lpVtbl->Release(ctx->d3d_context);
        ctx->d3d_device = NULL;
        ctx->d3d_context = NULL;
        return -1;
    }

    /* Reset device manager with our D3D11 device */
    hr = ctx->dxgi_manager->lpVtbl->ResetDevice(
        ctx->dxgi_manager, (IUnknown *)ctx->d3d_device, ctx->dxgi_reset_token);
    if (FAILED(hr)) {
        fprintf(stderr, "MF Decoder: ResetDevice failed: 0x%08lx\n", hr);
        ctx->dxgi_manager->lpVtbl->Release(ctx->dxgi_manager);
        ctx->d3d_device->lpVtbl->Release(ctx->d3d_device);
        ctx->d3d_context->lpVtbl->Release(ctx->d3d_context);
        ctx->dxgi_manager = NULL;
        ctx->d3d_device = NULL;
        ctx->d3d_context = NULL;
        return -1;
    }

    return 0;
}

static int mf_create_decoder(mf_decoder_ctx_t *ctx) {
    HRESULT hr;
    IMFActivate **activates = NULL;
    UINT32 count = 0;
    MFT_REGISTER_TYPE_INFO input_type;

    /* Set up input type info */
    input_type.guidMajorType = MFMediaType_Video;
    input_type.guidSubtype = (ctx->codec == CODEC_H265) ? MFVideoFormat_HEVC : MFVideoFormat_H264;

    /* Find hardware decoder first, then software */
    UINT32 flags = MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_SORTANDFILTER;
    if (ctx->d3d_device) {
        flags |= MFT_ENUM_FLAG_HARDWARE;
    }

    hr = MFTEnumEx(
        MFT_CATEGORY_VIDEO_DECODER,
        flags,
        &input_type,
        NULL,               /* Any output type */
        &activates,
        &count);

    if (FAILED(hr) || count == 0) {
        /* Try without hardware flag */
        hr = MFTEnumEx(
            MFT_CATEGORY_VIDEO_DECODER,
            MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_SORTANDFILTER,
            &input_type,
            NULL,
            &activates,
            &count);

        if (FAILED(hr) || count == 0) {
            fprintf(stderr, "MF Decoder: No decoder found for codec\n");
            return -1;
        }
    }

    /* Activate first decoder */
    hr = activates[0]->lpVtbl->ActivateObject(
        activates[0], &IID_IMFTransform, (void **)&ctx->decoder);

    /* Release activates */
    for (UINT32 i = 0; i < count; i++) {
        activates[i]->lpVtbl->Release(activates[i]);
    }
    CoTaskMemFree(activates);

    if (FAILED(hr)) {
        fprintf(stderr, "MF Decoder: ActivateObject failed: 0x%08lx\n", hr);
        return -1;
    }

    /* Set D3D11 device manager on decoder (for hardware decode) */
    if (ctx->dxgi_manager) {
        hr = ctx->decoder->lpVtbl->ProcessMessage(
            ctx->decoder, MFT_MESSAGE_SET_D3D_MANAGER,
            (ULONG_PTR)ctx->dxgi_manager);
        if (FAILED(hr)) {
            fprintf(stderr, "MF Decoder: Failed to set D3D manager: 0x%08lx\n", hr);
            /* Continue without D3D - will use software */
        }
    }

    return 0;
}

static int mf_configure_decoder(mf_decoder_ctx_t *ctx) {
    HRESULT hr;

    /* Create and set input type */
    hr = MFCreateMediaType(&ctx->input_type);
    if (FAILED(hr)) {
        return -1;
    }

    ctx->input_type->lpVtbl->SetGUID(ctx->input_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ctx->input_type->lpVtbl->SetGUID(ctx->input_type, &MF_MT_SUBTYPE,
        (ctx->codec == CODEC_H265) ? &MFVideoFormat_HEVC : &MFVideoFormat_H264);
    ctx->input_type->lpVtbl->SetUINT32(ctx->input_type, &MF_MT_INTERLACE_MODE,
        MFVideoInterlace_Progressive);

    /* Set frame size if known */
    if (ctx->width > 0 && ctx->height > 0) {
        MFSetAttributeSize(ctx->input_type, &MF_MT_FRAME_SIZE,
                          ctx->width, ctx->height);
    }

    hr = ctx->decoder->lpVtbl->SetInputType(ctx->decoder, 0, ctx->input_type, 0);
    if (FAILED(hr)) {
        fprintf(stderr, "MF Decoder: SetInputType failed: 0x%08lx\n", hr);
        return -1;
    }

    /* Get preferred output type (NV12) */
    DWORD output_index = 0;
    while (true) {
        IMFMediaType *out_type = NULL;
        hr = ctx->decoder->lpVtbl->GetOutputAvailableType(
            ctx->decoder, 0, output_index, &out_type);

        if (hr == MF_E_NO_MORE_TYPES) {
            break;
        }
        if (FAILED(hr)) {
            output_index++;
            continue;
        }

        GUID subtype;
        out_type->lpVtbl->GetGUID(out_type, &MF_MT_SUBTYPE, &subtype);

        if (IsEqualGUID(&subtype, &MFVideoFormat_NV12)) {
            ctx->output_type = out_type;
            break;
        }

        out_type->lpVtbl->Release(out_type);
        output_index++;
    }

    if (!ctx->output_type) {
        /* Fall back to first available output type */
        hr = ctx->decoder->lpVtbl->GetOutputAvailableType(
            ctx->decoder, 0, 0, &ctx->output_type);
        if (FAILED(hr)) {
            fprintf(stderr, "MF Decoder: No output type available\n");
            return -1;
        }
    }

    hr = ctx->decoder->lpVtbl->SetOutputType(ctx->decoder, 0, ctx->output_type, 0);
    if (FAILED(hr)) {
        fprintf(stderr, "MF Decoder: SetOutputType failed: 0x%08lx\n", hr);
        return -1;
    }

    /* Start streaming */
    hr = ctx->decoder->lpVtbl->ProcessMessage(
        ctx->decoder, MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    if (FAILED(hr)) {
        fprintf(stderr, "MF Decoder: BEGIN_STREAMING failed: 0x%08lx\n", hr);
    }

    hr = ctx->decoder->lpVtbl->ProcessMessage(
        ctx->decoder, MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    if (FAILED(hr)) {
        fprintf(stderr, "MF Decoder: START_OF_STREAM failed: 0x%08lx\n", hr);
    }

    return 0;
}

/* ============================================================================
 * Decoding
 * ============================================================================ */

int rootstream_decode_frame(rootstream_ctx_t *ctx,
                            const uint8_t *in, size_t in_size,
                            frame_buffer_t *out) {
    mf_decoder_ctx_t *mf = (mf_decoder_ctx_t *)ctx->decoder.backend_ctx;
    HRESULT hr;
    IMFSample *input_sample = NULL;
    IMFMediaBuffer *input_buffer = NULL;
    MFT_OUTPUT_DATA_BUFFER output_buffer = {0};
    DWORD status = 0;

    if (!mf || !mf->initialized) {
        return -1;
    }

    /* Create input sample */
    hr = MFCreateSample(&input_sample);
    if (FAILED(hr)) {
        return -1;
    }

    hr = MFCreateMemoryBuffer((DWORD)in_size, &input_buffer);
    if (FAILED(hr)) {
        input_sample->lpVtbl->Release(input_sample);
        return -1;
    }

    /* Copy input data */
    BYTE *buf;
    DWORD max_len;
    hr = input_buffer->lpVtbl->Lock(input_buffer, &buf, &max_len, NULL);
    if (SUCCEEDED(hr)) {
        memcpy(buf, in, in_size);
        input_buffer->lpVtbl->Unlock(input_buffer);
        input_buffer->lpVtbl->SetCurrentLength(input_buffer, (DWORD)in_size);
    }

    hr = input_sample->lpVtbl->AddBuffer(input_sample, input_buffer);
    input_buffer->lpVtbl->Release(input_buffer);

    if (FAILED(hr)) {
        input_sample->lpVtbl->Release(input_sample);
        return -1;
    }

    /* Submit to decoder */
    hr = mf->decoder->lpVtbl->ProcessInput(mf->decoder, 0, input_sample, 0);
    input_sample->lpVtbl->Release(input_sample);

    if (FAILED(hr) && hr != MF_E_NOTACCEPTING) {
        fprintf(stderr, "MF Decoder: ProcessInput failed: 0x%08lx\n", hr);
        return -1;
    }

    /* Get output */
    output_buffer.dwStreamID = 0;
    output_buffer.pSample = NULL;
    output_buffer.dwStatus = 0;
    output_buffer.pEvents = NULL;

    /* Create output sample if decoder doesn't provide one */
    IMFSample *output_sample = NULL;
    MFT_OUTPUT_STREAM_INFO stream_info;
    hr = mf->decoder->lpVtbl->GetOutputStreamInfo(mf->decoder, 0, &stream_info);

    if (SUCCEEDED(hr) && !(stream_info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)) {
        IMFMediaBuffer *out_buffer = NULL;
        hr = MFCreateSample(&output_sample);
        if (SUCCEEDED(hr)) {
            hr = MFCreateMemoryBuffer(stream_info.cbSize, &out_buffer);
            if (SUCCEEDED(hr)) {
                output_sample->lpVtbl->AddBuffer(output_sample, out_buffer);
                out_buffer->lpVtbl->Release(out_buffer);
            }
        }
        output_buffer.pSample = output_sample;
    }

    hr = mf->decoder->lpVtbl->ProcessOutput(mf->decoder, 0, 1, &output_buffer, &status);

    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
        /* Need more input data */
        if (output_sample) output_sample->lpVtbl->Release(output_sample);
        return 0;
    }

    if (FAILED(hr)) {
        if (output_sample) output_sample->lpVtbl->Release(output_sample);
        if (output_buffer.pEvents) output_buffer.pEvents->lpVtbl->Release(output_buffer.pEvents);
        return -1;
    }

    /* Extract frame data */
    if (output_buffer.pSample) {
        IMFMediaBuffer *media_buffer = NULL;
        hr = output_buffer.pSample->lpVtbl->ConvertToContiguousBuffer(
            output_buffer.pSample, &media_buffer);

        if (SUCCEEDED(hr)) {
            BYTE *data;
            DWORD data_len;
            hr = media_buffer->lpVtbl->Lock(media_buffer, &data, NULL, &data_len);

            if (SUCCEEDED(hr)) {
                /* Copy to output frame buffer */
                size_t copy_size = (data_len < mf->frame_buffer_size) ?
                                   data_len : mf->frame_buffer_size;
                memcpy(mf->frame_buffer, data, copy_size);

                /* Fill output frame info */
                out->data = mf->frame_buffer;
                out->size = (uint32_t)copy_size;
                out->width = mf->width;
                out->height = mf->height;
                out->pitch = mf->width;  /* NV12 Y-plane pitch */
                out->format = 0x3231564E;  /* NV12 fourcc */

                media_buffer->lpVtbl->Unlock(media_buffer);
            }
            media_buffer->lpVtbl->Release(media_buffer);
        }

        if (output_buffer.pSample != output_sample) {
            output_buffer.pSample->lpVtbl->Release(output_buffer.pSample);
        }
    }

    if (output_sample) {
        output_sample->lpVtbl->Release(output_sample);
    }
    if (output_buffer.pEvents) {
        output_buffer.pEvents->lpVtbl->Release(output_buffer.pEvents);
    }

    return 0;
}

/* ============================================================================
 * Cleanup
 * ============================================================================ */

static void mf_release_resources(mf_decoder_ctx_t *ctx) {
    if (ctx->decoder) {
        ctx->decoder->lpVtbl->ProcessMessage(
            ctx->decoder, MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
        ctx->decoder->lpVtbl->ProcessMessage(
            ctx->decoder, MFT_MESSAGE_COMMAND_DRAIN, 0);
        ctx->decoder->lpVtbl->Release(ctx->decoder);
    }
    if (ctx->input_type) {
        ctx->input_type->lpVtbl->Release(ctx->input_type);
    }
    if (ctx->output_type) {
        ctx->output_type->lpVtbl->Release(ctx->output_type);
    }
    if (ctx->dxgi_manager) {
        ctx->dxgi_manager->lpVtbl->Release(ctx->dxgi_manager);
    }
    if (ctx->d3d_context) {
        ctx->d3d_context->lpVtbl->Release(ctx->d3d_context);
    }
    if (ctx->d3d_device) {
        ctx->d3d_device->lpVtbl->Release(ctx->d3d_device);
    }
    if (ctx->frame_buffer) {
        free(ctx->frame_buffer);
    }
    if (ctx->mf_started) {
        MFShutdown();
    }
}

void rootstream_decoder_cleanup(rootstream_ctx_t *ctx) {
    mf_decoder_ctx_t *mf = (mf_decoder_ctx_t *)ctx->decoder.backend_ctx;

    if (!mf) {
        return;
    }

    mf_release_resources(mf);
    free(mf);
    ctx->decoder.backend_ctx = NULL;

    printf("MF Decoder: Cleanup complete\n");
}

#endif /* _WIN32 */
