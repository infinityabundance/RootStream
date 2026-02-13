/*
 * Unit tests for Proton Renderer
 */

#include <QtTest/QtTest>
#include "../../src/renderer/proton_detector.h"
#include "../../src/renderer/proton_renderer.h"
#include "../../src/renderer/proton_game_db.h"
#include "../../src/renderer/proton_settings.h"
#include "../../src/renderer/dxvk_interop.h"
#include "../../src/renderer/vkd3d_interop.h"

class TestProtonRenderer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup test environment
    }

    void cleanupTestCase() {
        // Cleanup
    }

    /**
     * Test Proton version parsing
     */
    void testProtonVersionParsing() {
        proton_version_t version;
        
        // Test simple version
        QVERIFY(proton_parse_version("8.3", &version));
        QCOMPARE(version.major, 8u);
        QCOMPARE(version.minor, 3u);
        QCOMPARE(version.patch, 0u);
        
        // Test version with patch
        QVERIFY(proton_parse_version("1.10.2", &version));
        QCOMPARE(version.major, 1u);
        QCOMPARE(version.minor, 10u);
        QCOMPARE(version.patch, 2u);
        
        // Test version with suffix
        QVERIFY(proton_parse_version("9.0-GE", &version));
        QCOMPARE(version.major, 9u);
        QCOMPARE(version.minor, 0u);
        QCOMPARE(QString(version.suffix), QString("GE"));
        
        // Test invalid version
        QVERIFY(!proton_parse_version("invalid", &version));
        QVERIFY(!proton_parse_version(nullptr, &version));
    }

    /**
     * Test Proton detection without Proton environment
     */
    void testProtonDetectionWithoutEnvironment() {
        // Clear Proton environment variables
        unsetenv("PROTON_VERSION");
        unsetenv("WINEPREFIX");
        unsetenv("WINE_PREFIX");
        
        proton_info_t info;
        bool detected = proton_detect(&info);
        
        // Should not detect Proton without environment
        QVERIFY(!detected || !info.is_running_under_proton);
    }

    /**
     * Test Proton detection with mocked environment
     */
    void testProtonDetectionWithEnvironment() {
        // Set mock Proton environment
        setenv("PROTON_VERSION", "8.3", 1);
        setenv("WINEPREFIX", "/tmp/test_wineprefix", 1);
        
        proton_info_t info;
        bool detected = proton_detect(&info);
        
        QVERIFY(detected);
        QVERIFY(info.is_running_under_proton);
        QCOMPARE(QString(info.proton_version), QString("8.3"));
        QCOMPARE(QString(info.wine_prefix_path), QString("/tmp/test_wineprefix"));
        
        // Cleanup
        unsetenv("PROTON_VERSION");
        unsetenv("WINEPREFIX");
    }

    /**
     * Test DXVK detection
     */
    void testDXVKDetection() {
        setenv("PROTON_VERSION", "8.3", 1);
        setenv("DXVK_HUD", "fps", 1);
        setenv("DXVK_VERSION", "1.10.3", 1);
        
        proton_info_t info;
        bool detected = proton_detect(&info);
        
        QVERIFY(detected);
        QVERIFY(info.has_dxvk);
        QCOMPARE(info.dxvk_version.major, 1u);
        QCOMPARE(info.dxvk_version.minor, 10u);
        QCOMPARE(info.dxvk_version.patch, 3u);
        
        // Cleanup
        unsetenv("PROTON_VERSION");
        unsetenv("DXVK_HUD");
        unsetenv("DXVK_VERSION");
    }

    /**
     * Test VKD3D detection
     */
    void testVKD3DDetection() {
        setenv("PROTON_VERSION", "8.3", 1);
        setenv("VKD3D_SHADER_DEBUG", "1", 1);
        setenv("VKD3D_VERSION", "1.2", 1);
        
        proton_info_t info;
        bool detected = proton_detect(&info);
        
        QVERIFY(detected);
        QVERIFY(info.has_vkd3d);
        QVERIFY(info.vkd3d_debug_enabled);
        QCOMPARE(info.vkd3d_version.major, 1u);
        QCOMPARE(info.vkd3d_version.minor, 2u);
        
        // Cleanup
        unsetenv("PROTON_VERSION");
        unsetenv("VKD3D_SHADER_DEBUG");
        unsetenv("VKD3D_VERSION");
    }

    /**
     * Test Proton info string generation
     */
    void testProtonInfoString() {
        proton_info_t info;
        memset(&info, 0, sizeof(info));
        info.is_running_under_proton = true;
        strncpy(info.proton_version, "8.3", sizeof(info.proton_version));
        info.has_dxvk = true;
        info.dxvk_version.major = 1;
        info.dxvk_version.minor = 10;
        
        char buf[1024];
        int len = proton_info_to_string(&info, buf, sizeof(buf));
        
        QVERIFY(len > 0);
        QVERIFY(strstr(buf, "8.3") != nullptr);
        QVERIFY(strstr(buf, "DXVK") != nullptr);
    }

    /**
     * Test game database lookup
     */
    void testGameDatabaseLookup() {
        // Test known game (Dota 2)
        const game_workaround_t *workarounds[10];
        int count = proton_game_db_lookup(570, workarounds, 10);
        
        QVERIFY(count > 0);
        QCOMPARE(workarounds[0]->steam_app_id, 570u);
        QCOMPARE(QString(workarounds[0]->game_name), QString("Dota 2"));
        QVERIFY(workarounds[0]->requires_async_compile);
        
        // Test unknown game
        count = proton_game_db_lookup(999999, workarounds, 10);
        QCOMPARE(count, 0);
    }

    /**
     * Test game database count
     */
    void testGameDatabaseCount() {
        int count = proton_game_db_get_count();
        QVERIFY(count > 0);
        QVERIFY(count < 1000);  // Sanity check
    }

    /**
     * Test game database index access
     */
    void testGameDatabaseIndexAccess() {
        int count = proton_game_db_get_count();
        
        // Test valid index
        const game_workaround_t *game = proton_game_db_get_by_index(0);
        QVERIFY(game != nullptr);
        QVERIFY(game->steam_app_id > 0);
        
        // Test invalid index
        game = proton_game_db_get_by_index(-1);
        QVERIFY(game == nullptr);
        
        game = proton_game_db_get_by_index(count);
        QVERIFY(game == nullptr);
    }

    /**
     * Test settings default values
     */
    void testSettingsDefaults() {
        proton_settings_t settings;
        proton_settings_get_default(&settings);
        
        QVERIFY(settings.enable_dxvk);
        QVERIFY(settings.enable_vkd3d);
        QVERIFY(settings.enable_async_shader_compile);
        QVERIFY(!settings.enable_dxvk_hud);
        QCOMPARE(settings.shader_cache_max_mb, 1024);
        QCOMPARE(QString(settings.preferred_directx_version), QString("auto"));
    }

    /**
     * Test settings save and load
     */
    void testSettingsSaveLoad() {
        proton_settings_t settings;
        proton_settings_get_default(&settings);
        
        // Modify settings
        settings.enable_dxvk_hud = true;
        settings.shader_cache_max_mb = 2048;
        strncpy(settings.preferred_directx_version, "11", 
               sizeof(settings.preferred_directx_version));
        
        // Save
        int result = proton_settings_save(&settings);
        if (result == 0) {
            // Load
            proton_settings_t loaded;
            result = proton_settings_load(&loaded);
            
            if (result == 0) {
                QVERIFY(loaded.enable_dxvk_hud);
                QCOMPARE(loaded.shader_cache_max_mb, 2048);
                QCOMPARE(QString(loaded.preferred_directx_version), QString("11"));
            }
        }
    }

    /**
     * Test DXVK interop initialization
     */
    void testDXVKInterop() {
        // Set DXVK environment
        setenv("DXVK_VERSION", "1.10.3", 1);
        
        dxvk_adapter_t *adapter = dxvk_init_from_env();
        
        if (adapter) {
            char version[64];
            int result = dxvk_query_version(adapter, version, sizeof(version));
            QCOMPARE(result, 0);
            
            dxvk_cleanup(adapter);
        }
        
        unsetenv("DXVK_VERSION");
    }

    /**
     * Test VKD3D interop initialization
     */
    void testVKD3DInterop() {
        // Set VKD3D environment
        setenv("VKD3D_VERSION", "1.2", 1);
        
        vkd3d_context_t *context = vkd3d_init_from_env();
        
        if (context) {
            char version[64];
            int result = vkd3d_query_version(context, version, sizeof(version));
            QCOMPARE(result, 0);
            
            vkd3d_cleanup(context);
        }
        
        unsetenv("VKD3D_VERSION");
    }

    /**
     * Test Proton availability check
     */
    void testProtonAvailability() {
        // Without Proton environment
        unsetenv("PROTON_VERSION");
        unsetenv("WINEPREFIX");
        QVERIFY(!proton_is_available());
        
        // With Proton environment
        setenv("PROTON_VERSION", "8.3", 1);
        QVERIFY(proton_is_available());
        unsetenv("PROTON_VERSION");
    }
};

QTEST_MAIN(TestProtonRenderer)
#include "test_proton_renderer.moc"
