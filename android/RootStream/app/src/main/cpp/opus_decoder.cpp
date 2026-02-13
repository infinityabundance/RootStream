// Opus Decoder Native Implementation
// TODO: Implement Opus audio decoding
// Phase 22.2.6

#include <jni.h>
#include <android/log.h>
// #include <opus.h>

#define LOG_TAG "OpusDecoder"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_rootstream_audio_OpusDecoder_nativeCreate(
        JNIEnv* env,
        jobject /* this */,
        jint sampleRate,
        jint channels) {
    LOGI("OpusDecoder native create: rate=%d, channels=%d", sampleRate, channels);
    
    // TODO: Create Opus decoder
    // OpusDecoder* decoder = opus_decoder_create(sampleRate, channels, nullptr);
    
    return 0; // Stub
}

JNIEXPORT jshortArray JNICALL
Java_com_rootstream_audio_OpusDecoder_nativeDecode(
        JNIEnv* env,
        jobject /* this */,
        jlong handle,
        jbyteArray encodedData) {
    // TODO: Decode Opus frame to PCM
    
    jshortArray result = env->NewShortArray(0);
    return result;
}

JNIEXPORT void JNICALL
Java_com_rootstream_audio_OpusDecoder_nativeDestroy(
        JNIEnv* env,
        jobject /* this */,
        jlong handle) {
    // TODO: Destroy Opus decoder
    // opus_decoder_destroy((OpusDecoder*)handle);
}

}
