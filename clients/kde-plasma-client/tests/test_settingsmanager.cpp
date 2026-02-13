/*
 * Unit tests for SettingsManager
 */

#include <QtTest/QtTest>
#include "../src/settingsmanager.h"

class TestSettingsManager : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultSettings() {
        SettingsManager manager;
        
        QVERIFY(manager.getBitrate() > 0);
    }

    void testSetCodec() {
        SettingsManager manager;
        
        manager.setCodec("h265");
        QCOMPARE(manager.getCodec(), QString("h265"));
    }

    void testSetBitrate() {
        SettingsManager manager;
        
        manager.setBitrate(15000000);
        QCOMPARE(manager.getBitrate(), 15000000);
    }

    void testSaveLoad() {
        {
            SettingsManager manager;
            manager.setCodec("vp9");
            manager.setBitrate(20000000);
            manager.save();
        }
        
        {
            SettingsManager manager;
            manager.load();
            QCOMPARE(manager.getCodec(), QString("vp9"));
            QCOMPARE(manager.getBitrate(), 20000000);
        }
    }
};

QTEST_MAIN(TestSettingsManager)
#include "test_settingsmanager.moc"
