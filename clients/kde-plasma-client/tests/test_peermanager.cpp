/*
 * Unit tests for PeerManager
 */

#include <QtTest/QtTest>
#include "../src/peermanager.h"
#include "../src/rootstreamclient.h"

class TestPeerManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup
    }

    void testAddManualPeer() {
        RootStreamClient client;
        PeerManager manager(&client);
        
        QCOMPARE(manager.rowCount(), 0);
        
        manager.addManualPeer("testkey@testhost");
        
        QCOMPARE(manager.rowCount(), 1);
        QCOMPARE(manager.data(manager.index(0), PeerManager::HostnameRole).toString(), 
                 QString("testhost"));
    }

    void testRemovePeer() {
        RootStreamClient client;
        PeerManager manager(&client);
        
        manager.addManualPeer("testkey@testhost");
        QCOMPARE(manager.rowCount(), 1);
        
        manager.removePeer(0);
        QCOMPARE(manager.rowCount(), 0);
    }

    void testClearPeers() {
        RootStreamClient client;
        PeerManager manager(&client);
        
        manager.addManualPeer("key1@host1");
        manager.addManualPeer("key2@host2");
        QCOMPARE(manager.rowCount(), 2);
        
        manager.clearPeers();
        QCOMPARE(manager.rowCount(), 0);
    }

    void cleanupTestCase() {
        // Cleanup
    }
};

QTEST_MAIN(TestPeerManager)
#include "test_peermanager.moc"
