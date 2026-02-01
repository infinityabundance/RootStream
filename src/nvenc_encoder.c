/*
 * nvenc_encoder.c - NVIDIA NVENC hardware encoding
 *
 * Native NVIDIA GPU encoding using Video Codec SDK.
 * Provides better quality and lower latency than VA-API/VDPAU wrapper.
 *
 * Requires:
 * - NVIDIA GPU with NVENC support (Kepler or newer)
 * - NVIDIA Video Codec SDK headers
 * - CUDA driver (no CUDA Toolkit needed for encoding)
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef HAVE_NVENC

#include <dlfcn.h>

/* NVENC SDK headers */
#include <nvEncodeAPI.h>

/* CUDA minimal headers for context */
#include <cuda.h>

typedef struct {
    /* NVENC instance */
    void *encoder;
    NV_ENCODE_API_FUNCTION_LIST nvenc_api;

    /* CUDA context */
    CUcontext cuda_ctx;
    CUdevice cuda_device;

    /* Encoding resources */
    void *input_buffer;
    void *output_buffer;
    NV_ENC_REGISTERED_PTR registered_resource;

    /* Configuration */
    int width;
    int height;
    int fps;
    int bitrate;

    /* SDK library handle */
    void *nvenc_lib;
    void *cuda_lib;
} nvenc_ctx_t;

/* CUDA function pointers */
static CUresult (*cuInit)(unsigned int Flags) = NULL;
static CUresult (*cuDeviceGet)(CUdevice *device, int ordinal) = NULL;
static CUresult (*cuCtxCreate)(CUcontext *pctx, unsigned int flags, CUdevice dev) = NULL;
static CUresult (*cuCtxDestroy)(CUcontext ctx) = NULL;
static CUresult (*cuMemAlloc)(CUdeviceptr *dptr, size_t bytesize) = NULL;
static CUresult (*cuMemFree)(CUdeviceptr dptr) = NULL;
static CUresult (*cuMemcpy2D)(const CUDA_MEMCPY2D *pCopy) = NULL;

/*
 * Load CUDA library dynamically
 */
static int nvenc_load_cuda(nvenc_ctx_t *nv) {
    /* Try loading CUDA driver library */
    nv->cuda_lib = dlopen("libcuda.so.1", RTLD_NOW);
    if (!nv->cuda_lib) {
        nv->cuda_lib = dlopen("libcuda.so", RTLD_NOW);
    }

    if (!nv->cuda_lib) {
        fprintf(stderr, "ERROR: Cannot load CUDA driver library\n");
        fprintf(stderr, "HINT: Install nvidia-driver or nvidia-utils\n");
        return -1;
    }

    /* Load function pointers */
    cuInit = dlsym(nv->cuda_lib, "cuInit");
    cuDeviceGet = dlsym(nv->cuda_lib, "cuDeviceGet");
    cuCtxCreate = dlsym(nv->cuda_lib, "cuCtxCreate_v2");
    cuCtxDestroy = dlsym(nv->cuda_lib, "cuCtxDestroy_v2");
    cuMemAlloc = dlsym(nv->cuda_lib, "cuMemAlloc_v2");
    cuMemFree = dlsym(nv->cuda_lib, "cuMemFree_v2");
    cuMemcpy2D = dlsym(nv->cuda_lib, "cuMemcpy2D_v2");

    if (!cuInit || !cuDeviceGet || !cuCtxCreate || !cuCtxDestroy ||
        !cuMemAlloc || !cuMemFree || !cuMemcpy2D) {
        fprintf(stderr, "ERROR: Failed to load CUDA symbols\n");
        dlclose(nv->cuda_lib);
        return -1;
    }

    return 0;
}

/*
 * Load NVENC SDK library dynamically
 */
static int nvenc_load_sdk(nvenc_ctx_t *nv) {
    /* Try loading NVENC library */
    nv->nvenc_lib = dlopen("libnvidia-encode.so.1", RTLD_NOW);
    if (!nv->nvenc_lib) {
        nv->nvenc_lib = dlopen("libnvidia-encode.so", RTLD_NOW);
    }

    if (!nv->nvenc_lib) {
        fprintf(stderr, "ERROR: Cannot load NVENC library\n");
        fprintf(stderr, "HINT: Install nvidia-driver or libnvidia-encode\n");
        return -1;
    }

    /* Get API function list */
    typedef NVENCSTATUS (NVENCAPI *NvEncodeAPICreateInstance_t)(NV_ENCODE_API_FUNCTION_LIST *functionList);
    NvEncodeAPICreateInstance_t NvEncodeAPICreateInstance;

    NvEncodeAPICreateInstance = dlsym(nv->nvenc_lib, "NvEncodeAPICreateInstance");
    if (!NvEncodeAPICreateInstance) {
        fprintf(stderr, "ERROR: Cannot find NvEncodeAPICreateInstance\n");
        dlclose(nv->nvenc_lib);
        return -1;
    }

    /* Initialize API function list */
    memset(&nv->nvenc_api, 0, sizeof(NV_ENCODE_API_FUNCTION_LIST));
    nv->nvenc_api.version = NV_ENCODE_API_FUNCTION_LIST_VER;

    NVENCSTATUS status = NvEncodeAPICreateInstance(&nv->nvenc_api);
    if (status != NV_ENC_SUCCESS) {
        fprintf(stderr, "ERROR: NvEncodeAPICreateInstance failed: %d\n", status);
        dlclose(nv->nvenc_lib);
        return -1;
    }

    return 0;
}

/*
 * Initialize NVENC encoder
 */
int rootstream_encoder_init_nvenc(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context\n");
        return -1;
    }

    /* Allocate NVENC context */
    nvenc_ctx_t *nv = calloc(1, sizeof(nvenc_ctx_t));
    if (!nv) {
        fprintf(stderr, "ERROR: Cannot allocate NVENC context\n");
        return -1;
    }

    /* Load CUDA library */
    if (nvenc_load_cuda(nv) < 0) {
        free(nv);
        return -1;
    }

    /* Initialize CUDA */
    CUresult cu_status = cuInit(0);
    if (cu_status != CUDA_SUCCESS) {
        fprintf(stderr, "ERROR: cuInit failed: %d\n", cu_status);
        dlclose(nv->cuda_lib);
        free(nv);
        return -1;
    }

    /* Get CUDA device */
    cu_status = cuDeviceGet(&nv->cuda_device, 0);
    if (cu_status != CUDA_SUCCESS) {
        fprintf(stderr, "ERROR: cuDeviceGet failed: %d\n", cu_status);
        dlclose(nv->cuda_lib);
        free(nv);
        return -1;
    }

    /* Create CUDA context */
    cu_status = cuCtxCreate(&nv->cuda_ctx, 0, nv->cuda_device);
    if (cu_status != CUDA_SUCCESS) {
        fprintf(stderr, "ERROR: cuCtxCreate failed: %d\n", cu_status);
        dlclose(nv->cuda_lib);
        free(nv);
        return -1;
    }

    printf("✓ CUDA initialized (device 0)\n");

    /* Load NVENC SDK */
    if (nvenc_load_sdk(nv) < 0) {
        cuCtxDestroy(nv->cuda_ctx);
        dlclose(nv->cuda_lib);
        free(nv);
        return -1;
    }

    /* Open NVENC session */
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS session_params = {0};
    session_params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    session_params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
    session_params.device = nv->cuda_ctx;
    session_params.apiVersion = NVENCAPI_VERSION;

    NVENCSTATUS status = nv->nvenc_api.nvEncOpenEncodeSessionEx(&session_params, &nv->encoder);
    if (status != NV_ENC_SUCCESS) {
        fprintf(stderr, "ERROR: nvEncOpenEncodeSessionEx failed: %d\n", status);
        dlclose(nv->nvenc_lib);
        cuCtxDestroy(nv->cuda_ctx);
        dlclose(nv->cuda_lib);
        free(nv);
        return -1;
    }

    printf("✓ NVENC session opened\n");

    /* Get encoder capabilities */
    NV_ENC_CAPS_PARAM caps_param = {0};
    caps_param.version = NV_ENC_CAPS_PARAM_VER;
    caps_param.capsToQuery = NV_ENC_CAPS_SUPPORTED_RATECONTROL_MODES;

    int caps_value = 0;
    status = nv->nvenc_api.nvEncGetEncodeCaps(nv->encoder, NV_ENC_CODEC_H264_GUID,
                                              &caps_param, &caps_value);
    if (status != NV_ENC_SUCCESS || caps_value == 0) {
        fprintf(stderr, "ERROR: H.264 encoding not supported\n");
        nv->nvenc_api.nvEncDestroyEncoder(nv->encoder);
        dlclose(nv->nvenc_lib);
        cuCtxDestroy(nv->cuda_ctx);
        dlclose(nv->cuda_lib);
        free(nv);
        return -1;
    }

    /* Set encoding parameters */
    nv->width = ctx->display.width;
    nv->height = ctx->display.height;
    nv->fps = ctx->display.refresh_rate;
    nv->bitrate = ctx->settings.video_bitrate;
    if (nv->bitrate == 0) {
        nv->bitrate = 10000000;  /* 10 Mbps default */
    }

    /* Initialize encoder */
    NV_ENC_INITIALIZE_PARAMS init_params = {0};
    init_params.version = NV_ENC_INITIALIZE_PARAMS_VER;
    init_params.encodeGUID = NV_ENC_CODEC_H264_GUID;
    init_params.presetGUID = NV_ENC_PRESET_P3_GUID;  /* Low latency, good quality */
    init_params.encodeWidth = nv->width;
    init_params.encodeHeight = nv->height;
    init_params.darWidth = nv->width;
    init_params.darHeight = nv->height;
    init_params.frameRateNum = nv->fps;
    init_params.frameRateDen = 1;
    init_params.enablePTD = 1;  /* Picture Type Decision */
    init_params.reportSliceOffsets = 0;
    init_params.enableSubFrameWrite = 0;
    init_params.maxEncodeWidth = nv->width;
    init_params.maxEncodeHeight = nv->height;

    /* Configure codec settings */
    NV_ENC_CONFIG encode_config = {0};
    encode_config.version = NV_ENC_CONFIG_VER;
    encode_config.profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;
    encode_config.gopLength = nv->fps * 2;  /* 2 second GOP */
    encode_config.frameIntervalP = 1;        /* All P-frames (low latency) */
    encode_config.frameFieldMode = NV_ENC_PARAMS_FRAME_FIELD_MODE_FRAME;
    encode_config.mvPrecision = NV_ENC_MV_PRECISION_QUARTER_PEL;

    /* Rate control */
    encode_config.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
    encode_config.rcParams.averageBitRate = nv->bitrate;
    encode_config.rcParams.maxBitRate = nv->bitrate;
    encode_config.rcParams.vbvBufferSize = nv->bitrate / nv->fps;
    encode_config.rcParams.vbvInitialDelay = nv->bitrate / (nv->fps * 2);
    encode_config.rcParams.enableMinQP = 0;
    encode_config.rcParams.enableMaxQP = 0;
    encode_config.rcParams.zeroReorderDelay = 1;  /* Low latency */

    /* H.264 specific settings */
    encode_config.encodeCodecConfig.h264Config.idrPeriod = encode_config.gopLength;
    encode_config.encodeCodecConfig.h264Config.sliceMode = 0;
    encode_config.encodeCodecConfig.h264Config.sliceModeData = 0;
    encode_config.encodeCodecConfig.h264Config.level = NV_ENC_LEVEL_AUTOSELECT;
    encode_config.encodeCodecConfig.h264Config.chromaFormatIDC = 1;  /* YUV 4:2:0 */
    encode_config.encodeCodecConfig.h264Config.outputBufferingPeriodSEI = 0;
    encode_config.encodeCodecConfig.h264Config.outputPictureTimingSEI = 0;
    encode_config.encodeCodecConfig.h264Config.outputAUD = 0;
    encode_config.encodeCodecConfig.h264Config.disableSPSPPS = 0;
    encode_config.encodeCodecConfig.h264Config.repeatSPSPPS = 1;
    encode_config.encodeCodecConfig.h264Config.enableIntraRefresh = 0;

    init_params.encodeConfig = &encode_config;
    init_params.tuningInfo = NV_ENC_TUNING_INFO_LOW_LATENCY;

    status = nv->nvenc_api.nvEncInitializeEncoder(nv->encoder, &init_params);
    if (status != NV_ENC_SUCCESS) {
        fprintf(stderr, "ERROR: nvEncInitializeEncoder failed: %d\n", status);
        nv->nvenc_api.nvEncDestroyEncoder(nv->encoder);
        dlclose(nv->nvenc_lib);
        cuCtxDestroy(nv->cuda_ctx);
        dlclose(nv->cuda_lib);
        free(nv);
        return -1;
    }

    /* Create input buffer (in CUDA device memory) */
    size_t frame_size = nv->width * nv->height * 4;  /* RGBA */
    cu_status = cuMemAlloc((CUdeviceptr*)&nv->input_buffer, frame_size);
    if (cu_status != CUDA_SUCCESS) {
        fprintf(stderr, "ERROR: cuMemAlloc failed for input buffer: %d\n", cu_status);
        nv->nvenc_api.nvEncDestroyEncoder(nv->encoder);
        dlclose(nv->nvenc_lib);
        cuCtxDestroy(nv->cuda_ctx);
        dlclose(nv->cuda_lib);
        free(nv);
        return -1;
    }

    /* Register input buffer with NVENC */
    NV_ENC_REGISTER_RESOURCE register_params = {0};
    register_params.version = NV_ENC_REGISTER_RESOURCE_VER;
    register_params.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
    register_params.resourceToRegister = nv->input_buffer;
    register_params.width = nv->width;
    register_params.height = nv->height;
    register_params.pitch = nv->width * 4;
    register_params.bufferFormat = NV_ENC_BUFFER_FORMAT_ABGR;

    status = nv->nvenc_api.nvEncRegisterResource(nv->encoder, &register_params);
    if (status != NV_ENC_SUCCESS) {
        fprintf(stderr, "ERROR: nvEncRegisterResource failed: %d\n", status);
        cuMemFree((CUdeviceptr)nv->input_buffer);
        nv->nvenc_api.nvEncDestroyEncoder(nv->encoder);
        dlclose(nv->nvenc_lib);
        cuCtxDestroy(nv->cuda_ctx);
        dlclose(nv->cuda_lib);
        free(nv);
        return -1;
    }

    nv->registered_resource = register_params.registeredResource;

    /* Create output bitstream buffer */
    NV_ENC_CREATE_BITSTREAM_BUFFER bitstream_params = {0};
    bitstream_params.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;

    status = nv->nvenc_api.nvEncCreateBitstreamBuffer(nv->encoder, &bitstream_params);
    if (status != NV_ENC_SUCCESS) {
        fprintf(stderr, "ERROR: nvEncCreateBitstreamBuffer failed: %d\n", status);
        nv->nvenc_api.nvEncUnregisterResource(nv->encoder, nv->registered_resource);
        cuMemFree((CUdeviceptr)nv->input_buffer);
        nv->nvenc_api.nvEncDestroyEncoder(nv->encoder);
        dlclose(nv->nvenc_lib);
        cuCtxDestroy(nv->cuda_ctx);
        dlclose(nv->cuda_lib);
        free(nv);
        return -1;
    }

    nv->output_buffer = bitstream_params.bitstreamBuffer;

    /* Store in context */
    ctx->encoder.type = ENCODER_NVENC;
    ctx->encoder.hw_ctx = nv;
    ctx->encoder.bitrate = nv->bitrate;
    ctx->encoder.framerate = nv->fps;
    ctx->encoder.low_latency = true;

    printf("✓ NVENC encoder ready: %dx%d @ %d fps, %d kbps\n",
           nv->width, nv->height, nv->fps, nv->bitrate / 1000);

    return 0;
}

/*
 * Encode a frame using NVENC
 */
int rootstream_encode_frame_nvenc(rootstream_ctx_t *ctx, frame_buffer_t *in,
                                  uint8_t *out, size_t *out_size) {
    if (!ctx || !in || !out || !out_size) {
        return -1;
    }

    nvenc_ctx_t *nv = (nvenc_ctx_t*)ctx->encoder.hw_ctx;
    if (!nv || !nv->encoder) {
        fprintf(stderr, "ERROR: NVENC encoder not initialized\n");
        return -1;
    }

    /* Upload frame to CUDA device memory */
    CUDA_MEMCPY2D copy_params = {0};
    copy_params.srcMemoryType = CU_MEMORYTYPE_HOST;
    copy_params.srcHost = in->data;
    copy_params.srcPitch = in->pitch;
    copy_params.dstMemoryType = CU_MEMORYTYPE_DEVICE;
    copy_params.dstDevice = (CUdeviceptr)nv->input_buffer;
    copy_params.dstPitch = nv->width * 4;
    copy_params.WidthInBytes = nv->width * 4;
    copy_params.Height = nv->height;

    CUresult cu_status = cuMemcpy2D(&copy_params);
    if (cu_status != CUDA_SUCCESS) {
        fprintf(stderr, "ERROR: cuMemcpy2D failed: %d\n", cu_status);
        return -1;
    }

    /* Map input resource */
    NV_ENC_MAP_INPUT_RESOURCE map_params = {0};
    map_params.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
    map_params.registeredResource = nv->registered_resource;

    NVENCSTATUS status = nv->nvenc_api.nvEncMapInputResource(nv->encoder, &map_params);
    if (status != NV_ENC_SUCCESS) {
        fprintf(stderr, "ERROR: nvEncMapInputResource failed: %d\n", status);
        return -1;
    }

    /* Encode frame */
    NV_ENC_PIC_PARAMS pic_params = {0};
    pic_params.version = NV_ENC_PIC_PARAMS_VER;
    pic_params.inputBuffer = map_params.mappedResource;
    pic_params.bufferFmt = map_params.mappedBufferFmt;
    pic_params.inputWidth = nv->width;
    pic_params.inputHeight = nv->height;
    pic_params.outputBitstream = nv->output_buffer;
    pic_params.completionEvent = NULL;
    pic_params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    pic_params.encodePicFlags = 0;

    status = nv->nvenc_api.nvEncEncodePicture(nv->encoder, &pic_params);
    if (status != NV_ENC_SUCCESS) {
        nv->nvenc_api.nvEncUnmapInputResource(nv->encoder, map_params.mappedResource);
        fprintf(stderr, "ERROR: nvEncEncodePicture failed: %d\n", status);
        return -1;
    }

    /* Unmap input */
    nv->nvenc_api.nvEncUnmapInputResource(nv->encoder, map_params.mappedResource);

    /* Lock output bitstream */
    NV_ENC_LOCK_BITSTREAM lock_params = {0};
    lock_params.version = NV_ENC_LOCK_BITSTREAM_VER;
    lock_params.outputBitstream = nv->output_buffer;

    status = nv->nvenc_api.nvEncLockBitstream(nv->encoder, &lock_params);
    if (status != NV_ENC_SUCCESS) {
        fprintf(stderr, "ERROR: nvEncLockBitstream failed: %d\n", status);
        return -1;
    }

    /* Copy encoded data */
    *out_size = lock_params.bitstreamSizeInBytes;
    memcpy(out, lock_params.bitstreamBufferPtr, *out_size);

    /* Unlock bitstream */
    nv->nvenc_api.nvEncUnlockBitstream(nv->encoder, nv->output_buffer);

    ctx->frames_encoded++;
    return 0;
}

/*
 * Cleanup NVENC encoder
 */
void rootstream_encoder_cleanup_nvenc(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->encoder.hw_ctx) {
        return;
    }

    nvenc_ctx_t *nv = (nvenc_ctx_t*)ctx->encoder.hw_ctx;

    if (nv->output_buffer && nv->nvenc_api.nvEncDestroyBitstreamBuffer) {
        nv->nvenc_api.nvEncDestroyBitstreamBuffer(nv->encoder, nv->output_buffer);
    }

    if (nv->registered_resource && nv->nvenc_api.nvEncUnregisterResource) {
        nv->nvenc_api.nvEncUnregisterResource(nv->encoder, nv->registered_resource);
    }

    if (nv->input_buffer && cuMemFree) {
        cuMemFree((CUdeviceptr)nv->input_buffer);
    }

    if (nv->encoder && nv->nvenc_api.nvEncDestroyEncoder) {
        nv->nvenc_api.nvEncDestroyEncoder(nv->encoder);
    }

    if (nv->cuda_ctx && cuCtxDestroy) {
        cuCtxDestroy(nv->cuda_ctx);
    }

    if (nv->nvenc_lib) {
        dlclose(nv->nvenc_lib);
    }

    if (nv->cuda_lib) {
        dlclose(nv->cuda_lib);
    }

    free(nv);
    ctx->encoder.hw_ctx = NULL;

    printf("✓ NVENC encoder cleanup complete\n");
}

/*
 * Check if NVENC is available
 */
bool rootstream_encoder_nvenc_available(void) {
    /* Check for NVIDIA device */
    void *cuda_lib = dlopen("libcuda.so.1", RTLD_NOW);
    if (!cuda_lib) {
        cuda_lib = dlopen("libcuda.so", RTLD_NOW);
    }

    if (!cuda_lib) {
        return false;
    }

    CUresult (*cuInit_check)(unsigned int) = dlsym(cuda_lib, "cuInit");
    CUresult (*cuDeviceGetCount_check)(int*) = dlsym(cuda_lib, "cuDeviceGetCount");

    if (!cuInit_check || !cuDeviceGetCount_check) {
        dlclose(cuda_lib);
        return false;
    }

    if (cuInit_check(0) != CUDA_SUCCESS) {
        dlclose(cuda_lib);
        return false;
    }

    int device_count = 0;
    if (cuDeviceGetCount_check(&device_count) != CUDA_SUCCESS || device_count == 0) {
        dlclose(cuda_lib);
        return false;
    }

    dlclose(cuda_lib);
    return true;
}

#else  /* !HAVE_NVENC */

/* Stub implementations when NVENC is not available */

int rootstream_encoder_init_nvenc(rootstream_ctx_t *ctx) {
    (void)ctx;
    fprintf(stderr, "ERROR: NVENC support not compiled in\n");
    return -1;
}

int rootstream_encode_frame_nvenc(rootstream_ctx_t *ctx, frame_buffer_t *in,
                                  uint8_t *out, size_t *out_size) {
    (void)ctx; (void)in; (void)out; (void)out_size;
    return -1;
}

void rootstream_encoder_cleanup_nvenc(rootstream_ctx_t *ctx) {
    (void)ctx;
}

bool rootstream_encoder_nvenc_available(void) {
    return false;
}

#endif  /* HAVE_NVENC */
