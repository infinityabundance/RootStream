/*
 * PeerManager - Peer Discovery and Connection Management
 */

#ifndef PEERMANAGER_H
#define PEERMANAGER_H

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QList>

class RootStreamClient;

struct PeerInfo {
    QString code;
    QString hostname;
    QString address;
    bool discovered;
};

class PeerManager : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    
public:
    enum PeerRoles {
        CodeRole = Qt::UserRole + 1,
        HostnameRole,
        AddressRole,
        DiscoveredRole
    };
    
    explicit PeerManager(RootStreamClient *client, QObject *parent = nullptr);
    
    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    // Peer management
    Q_INVOKABLE void startDiscovery();
    Q_INVOKABLE void stopDiscovery();
    Q_INVOKABLE void addManualPeer(const QString &code);
    Q_INVOKABLE void removePeer(int index);
    Q_INVOKABLE void clearPeers();
    
signals:
    void countChanged();
    void peerAdded(const QString &code);
    void peerRemoved(const QString &code);
    
private slots:
    void onPeerDiscovered(const QString &code, const QString &hostname);
    void onPeerLost(const QString &code);
    
private:
    RootStreamClient *m_client;
    QList<PeerInfo> m_peers;
    bool m_discovering;
    
    int findPeer(const QString &code) const;
};

#endif // PEERMANAGER_H
