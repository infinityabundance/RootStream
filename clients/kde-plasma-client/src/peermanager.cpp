/*
 * PeerManager Implementation
 */

#include "peermanager.h"
#include "rootstreamclient.h"
#include <QDebug>

PeerManager::PeerManager(RootStreamClient *client, QObject *parent)
    : QAbstractListModel(parent)
    , m_client(client)
    , m_discovering(false)
{
    // Connect to client signals
    connect(m_client, &RootStreamClient::peerDiscovered,
            this, &PeerManager::onPeerDiscovered);
    connect(m_client, &RootStreamClient::peerLost,
            this, &PeerManager::onPeerLost);
}

int PeerManager::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_peers.count();
}

QVariant PeerManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_peers.count())
        return QVariant();
    
    const PeerInfo &peer = m_peers.at(index.row());
    
    switch (role) {
    case CodeRole:
        return peer.code;
    case HostnameRole:
        return peer.hostname;
    case AddressRole:
        return peer.address;
    case DiscoveredRole:
        return peer.discovered;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PeerManager::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CodeRole] = "code";
    roles[HostnameRole] = "hostname";
    roles[AddressRole] = "address";
    roles[DiscoveredRole] = "discovered";
    return roles;
}

void PeerManager::startDiscovery()
{
    if (m_discovering) {
        qInfo() << "Discovery already running";
        return;
    }
    
    qInfo() << "Starting peer discovery";
    m_discovering = true;
    
    // In a full implementation, this would start mDNS discovery
    // For now, this is a placeholder
}

void PeerManager::stopDiscovery()
{
    if (!m_discovering) {
        return;
    }
    
    qInfo() << "Stopping peer discovery";
    m_discovering = false;
}

void PeerManager::addManualPeer(const QString &code)
{
    if (findPeer(code) >= 0) {
        qInfo() << "Peer already exists:" << code;
        return;
    }
    
    PeerInfo peer;
    peer.code = code;
    
    // Parse hostname from code (format: pubkey@hostname)
    int atIndex = code.indexOf('@');
    if (atIndex > 0) {
        peer.hostname = code.mid(atIndex + 1);
    } else {
        peer.hostname = "Unknown";
    }
    
    peer.address = peer.hostname;
    peer.discovered = false;
    
    beginInsertRows(QModelIndex(), m_peers.count(), m_peers.count());
    m_peers.append(peer);
    endInsertRows();
    
    emit countChanged();
    emit peerAdded(code);
    
    qInfo() << "Added manual peer:" << code;
}

void PeerManager::removePeer(int index)
{
    if (index < 0 || index >= m_peers.count()) {
        return;
    }
    
    QString code = m_peers.at(index).code;
    
    beginRemoveRows(QModelIndex(), index, index);
    m_peers.removeAt(index);
    endRemoveRows();
    
    emit countChanged();
    emit peerRemoved(code);
    
    qInfo() << "Removed peer:" << code;
}

void PeerManager::clearPeers()
{
    if (m_peers.isEmpty()) {
        return;
    }
    
    beginResetModel();
    m_peers.clear();
    endResetModel();
    
    emit countChanged();
    
    qInfo() << "Cleared all peers";
}

void PeerManager::onPeerDiscovered(const QString &code, const QString &hostname)
{
    int index = findPeer(code);
    if (index >= 0) {
        // Update existing peer
        m_peers[index].hostname = hostname;
        m_peers[index].discovered = true;
        QModelIndex modelIndex = this->index(index);
        emit dataChanged(modelIndex, modelIndex);
    } else {
        // Add new peer
        PeerInfo peer;
        peer.code = code;
        peer.hostname = hostname;
        peer.address = hostname;
        peer.discovered = true;
        
        beginInsertRows(QModelIndex(), m_peers.count(), m_peers.count());
        m_peers.append(peer);
        endInsertRows();
        
        emit countChanged();
        emit peerAdded(code);
    }
    
    qInfo() << "Peer discovered:" << code << hostname;
}

void PeerManager::onPeerLost(const QString &code)
{
    int index = findPeer(code);
    if (index >= 0) {
        // Mark as not discovered (but keep in list)
        m_peers[index].discovered = false;
        QModelIndex modelIndex = this->index(index);
        emit dataChanged(modelIndex, modelIndex);
        
        qInfo() << "Peer lost:" << code;
    }
}

int PeerManager::findPeer(const QString &code) const
{
    for (int i = 0; i < m_peers.count(); i++) {
        if (m_peers.at(i).code == code) {
            return i;
        }
    }
    return -1;
}
