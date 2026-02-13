/*
 * Unit tests for Video Renderer
 */

#include <QtTest/QtTest>
#include "../../src/renderer/renderer.h"
#include "../../src/renderer/frame_buffer.h"
#include "../../src/renderer/color_space.h"

extern "C" {
    // Include C headers
}

class TestRenderer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup
    }

    /**
     * Test renderer creation
     */
    void testRendererCreate() {
        renderer_t *renderer = renderer_create(RENDERER_OPENGL, 1920, 1080);
        QVERIFY(renderer != nullptr);
        renderer_cleanup(renderer);
    }

    /**
     * Test invalid renderer creation
     */
    void testRendererCreateInvalid() {
        // Invalid dimensions
        renderer_t *renderer = renderer_create(RENDERER_OPENGL, 0, 0);
        QVERIFY(renderer == nullptr);
        
        renderer = renderer_create(RENDERER_OPENGL, -1, 1080);
        QVERIFY(renderer == nullptr);
    }

    /**
     * Test backend auto-detection
     */
    void testRendererAutoBackend() {
        renderer_t *renderer = renderer_create(RENDERER_AUTO, 1920, 1080);
        QVERIFY(renderer != nullptr);
        renderer_cleanup(renderer);
    }

    /**
     * Test frame buffer initialization
     */
    void testFrameBufferInit() {
        frame_buffer_t buffer;
        int result = frame_buffer_init(&buffer);
        QCOMPARE(result, 0);
        
        // Check initial count
        int count = frame_buffer_count(&buffer);
        QCOMPARE(count, 0);
        
        frame_buffer_cleanup(&buffer);
    }

    /**
     * Test frame buffer enqueue/dequeue
     */
    void testFrameBufferEnqueueDequeue() {
        frame_buffer_t buffer;
        frame_buffer_init(&buffer);
        
        // Create test frame
        frame_t frame;
        frame.width = 1920;
        frame.height = 1080;
        frame.format = FRAME_FORMAT_NV12;
        frame.size = frame.width * frame.height * 3 / 2;
        frame.data = (uint8_t*)malloc(frame.size);
        frame.timestamp_us = 1000000;
        frame.is_keyframe = true;
        
        // Fill with test pattern
        memset(frame.data, 128, frame.size);
        
        // Enqueue frame
        int result = frame_buffer_enqueue(&buffer, &frame);
        QCOMPARE(result, 0);
        
        // Check count
        int count = frame_buffer_count(&buffer);
        QCOMPARE(count, 1);
        
        // Dequeue frame
        frame_t *dequeued = frame_buffer_dequeue(&buffer);
        QVERIFY(dequeued != nullptr);
        QCOMPARE(dequeued->width, (uint32_t)1920);
        QCOMPARE(dequeued->height, (uint32_t)1080);
        QCOMPARE(dequeued->size, frame.size);
        
        // Free dequeued frame
        free(dequeued->data);
        free(dequeued);
        free(frame.data);
        
        frame_buffer_cleanup(&buffer);
    }

    /**
     * Test frame buffer overflow (frame dropping)
     */
    void testFrameBufferOverflow() {
        frame_buffer_t buffer;
        frame_buffer_init(&buffer);
        
        // Create test frame
        frame_t frame;
        frame.width = 640;
        frame.height = 480;
        frame.format = FRAME_FORMAT_NV12;
        frame.size = frame.width * frame.height * 3 / 2;
        frame.data = (uint8_t*)malloc(frame.size);
        frame.timestamp_us = 0;
        frame.is_keyframe = false;
        memset(frame.data, 0, frame.size);
        
        // Fill buffer beyond capacity
        for (int i = 0; i < 10; i++) {
            frame.timestamp_us = i * 16666; // ~60 FPS timestamps
            frame_buffer_enqueue(&buffer, &frame);
        }
        
        // Buffer size is limited, so some frames should be dropped
        int count = frame_buffer_count(&buffer);
        QVERIFY(count <= 4); // FRAME_BUFFER_SIZE is 4
        
        // Clean up
        while (frame_buffer_count(&buffer) > 0) {
            frame_t *f = frame_buffer_dequeue(&buffer);
            if (f) {
                free(f->data);
                free(f);
            }
        }
        
        free(frame.data);
        frame_buffer_cleanup(&buffer);
    }

    /**
     * Test color space conversion matrix
     */
    void testColorSpaceMatrix() {
        float matrix[9];
        color_space_get_yuv_to_rgb_matrix(matrix);
        
        // Check that matrix values are reasonable
        // BT.709 matrix should have these approximate values
        QVERIFY(qAbs(matrix[0] - 1.164f) < 0.01f);  // Y contribution to R
        QVERIFY(qAbs(matrix[1] - 1.164f) < 0.01f);  // Y contribution to G
        QVERIFY(qAbs(matrix[2] - 1.164f) < 0.01f);  // Y contribution to B
    }

    /**
     * Test color space offsets
     */
    void testColorSpaceOffsets() {
        float offsets[3];
        color_space_get_yuv_offsets(offsets);
        
        // Check offset values for limited range video
        QVERIFY(qAbs(offsets[0] - (16.0f / 255.0f)) < 0.01f);   // Y offset
        QVERIFY(qAbs(offsets[1] - (128.0f / 255.0f)) < 0.01f);  // U offset
        QVERIFY(qAbs(offsets[2] - (128.0f / 255.0f)) < 0.01f);  // V offset
    }

    /**
     * Test renderer metrics initialization
     */
    void testRendererMetrics() {
        renderer_t *renderer = renderer_create(RENDERER_OPENGL, 1920, 1080);
        QVERIFY(renderer != nullptr);
        
        struct renderer_metrics metrics = renderer_get_metrics(renderer);
        
        // Initial metrics should be zero
        QCOMPARE(metrics.total_frames, (uint64_t)0);
        QCOMPARE(metrics.frames_dropped, (uint64_t)0);
        QCOMPARE(metrics.fps, 0.0);
        
        renderer_cleanup(renderer);
    }

    /**
     * Test frame submission
     */
    void testFrameSubmission() {
        renderer_t *renderer = renderer_create(RENDERER_OPENGL, 1920, 1080);
        QVERIFY(renderer != nullptr);
        
        // Create test frame
        frame_t frame;
        frame.width = 1920;
        frame.height = 1080;
        frame.format = FRAME_FORMAT_NV12;
        frame.size = frame.width * frame.height * 3 / 2;
        frame.data = (uint8_t*)malloc(frame.size);
        frame.timestamp_us = 1000000;
        frame.is_keyframe = true;
        memset(frame.data, 128, frame.size);
        
        // Submit frame (should succeed even without init, just queues it)
        int result = renderer_submit_frame(renderer, &frame);
        QCOMPARE(result, 0);
        
        // Check metrics
        struct renderer_metrics metrics = renderer_get_metrics(renderer);
        QCOMPARE(metrics.total_frames, (uint64_t)1);
        
        free(frame.data);
        renderer_cleanup(renderer);
    }

    /**
     * Test error handling
     */
    void testErrorHandling() {
        renderer_t *renderer = renderer_create(RENDERER_OPENGL, 1920, 1080);
        QVERIFY(renderer != nullptr);
        
        // Initially no error
        const char *error = renderer_get_error(renderer);
        QVERIFY(error == nullptr);
        
        // Submit invalid frame
        int result = renderer_submit_frame(renderer, nullptr);
        QCOMPARE(result, -1);
        
        renderer_cleanup(renderer);
    }

    void cleanupTestCase() {
        // Cleanup
    }
};

QTEST_MAIN(TestRenderer)
#include "test_renderer.moc"
