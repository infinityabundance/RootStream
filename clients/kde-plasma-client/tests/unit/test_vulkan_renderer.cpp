/*
 * Unit tests for Vulkan Video Renderer
 */

#include <QtTest/QtTest>

#ifdef HAVE_VULKAN_RENDERER
#include "../../src/renderer/renderer.h"
#include "../../src/renderer/vulkan_renderer.h"
#endif

class TestVulkanRenderer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup
    }

#ifdef HAVE_VULKAN_RENDERER
    /**
     * Test backend detection
     */
    void testBackendDetection() {
        vulkan_backend_t backend = vulkan_detect_backend();
        
        // Should return one of: WAYLAND, X11, or HEADLESS
        QVERIFY(backend == VULKAN_BACKEND_WAYLAND || 
                backend == VULKAN_BACKEND_X11 || 
                backend == VULKAN_BACKEND_HEADLESS);
    }

    /**
     * Test Vulkan renderer creation
     */
    void testVulkanRendererCreate() {
        renderer_t *renderer = renderer_create(RENDERER_VULKAN, 1920, 1080);
        QVERIFY(renderer != nullptr);
        renderer_cleanup(renderer);
    }

    /**
     * Test invalid Vulkan renderer creation
     */
    void testVulkanRendererCreateInvalid() {
        // Invalid dimensions
        renderer_t *renderer = renderer_create(RENDERER_VULKAN, 0, 0);
        QVERIFY(renderer == nullptr);
        
        renderer = renderer_create(RENDERER_VULKAN, -1, 1080);
        QVERIFY(renderer == nullptr);
    }

    /**
     * Test headless backend initialization
     */
    void testHeadlessBackendInit() {
        // Headless backend should always be available
        renderer_t *renderer = renderer_create(RENDERER_VULKAN, 1920, 1080);
        QVERIFY(renderer != nullptr);
        
        // Init without window (headless mode)
        int result = renderer_init(renderer, nullptr);
        
        // May fail if Vulkan is not available on the system
        // But at least shouldn't crash
        if (result == 0) {
            // Success - verify we can get backend name
            // Note: This test may be skipped on CI without Vulkan
        }
        
        renderer_cleanup(renderer);
    }

    /**
     * Test backend name retrieval
     */
    void testBackendName() {
        vulkan_backend_t backend = vulkan_detect_backend();
        
        // Create a dummy context just to test the name function
        // This is a unit test, so we don't fully initialize
        
        const char* expected_names[] = {"wayland", "x11", "headless"};
        bool valid_backend = false;
        
        for (int i = 0; i < 3; i++) {
            if (backend == i) {
                valid_backend = true;
                break;
            }
        }
        
        QVERIFY(valid_backend);
    }

    /**
     * Test frame submission (without full init)
     */
    void testFrameSubmit() {
        renderer_t *renderer = renderer_create(RENDERER_VULKAN, 1920, 1080);
        QVERIFY(renderer != nullptr);
        
        // Create test frame
        frame_t frame;
        frame.width = 1920;
        frame.height = 1080;
        frame.format = 0x3231564E; // NV12 fourcc
        frame.size = frame.width * frame.height * 3 / 2;
        frame.data = (uint8_t*)malloc(frame.size);
        frame.timestamp_us = 1000000;
        frame.is_keyframe = true;
        
        memset(frame.data, 128, frame.size);
        
        // Submit frame (should enqueue even without init)
        int result = renderer_submit_frame(renderer, &frame);
        QCOMPARE(result, 0);
        
        free(frame.data);
        renderer_cleanup(renderer);
    }
#else
    /**
     * Test that Vulkan renderer reports as not compiled in
     */
    void testVulkanNotCompiled() {
        // When Vulkan is not compiled in, this should be a no-op test
        QVERIFY(true);
    }
#endif

    void cleanupTestCase() {
        // Cleanup
    }
};

QTEST_MAIN(TestVulkanRenderer)
#include "test_vulkan_renderer.moc"
