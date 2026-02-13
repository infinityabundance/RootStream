//
//  PeerDiscovery.swift
//  RootStream iOS
//
//  mDNS-based peer discovery using Network framework
//

import Foundation
import Network

@MainActor
class PeerDiscovery: ObservableObject {
    @Published var discoveredPeers: [Peer] = []
    
    private var browser: NWBrowser?
    private let serviceType = "_rootstream._tcp"
    private var peerMap: [String: Peer] = [:]
    
    func startDiscovery() {
        let parameters = NWParameters()
        parameters.includePeerToPeer = true
        
        let browser = NWBrowser(for: .bonjour(type: serviceType, domain: nil), using: parameters)
        self.browser = browser
        
        browser.stateUpdateHandler = { [weak self] state in
            Task { @MainActor in
                switch state {
                case .ready:
                    print("Browser ready")
                case .failed(let error):
                    print("Browser failed: \(error)")
                case .cancelled:
                    print("Browser cancelled")
                default:
                    break
                }
            }
        }
        
        browser.browseResultsChangedHandler = { [weak self] results, changes in
            Task { @MainActor in
                guard let self = self else { return }
                
                for result in results {
                    switch result.endpoint {
                    case .service(let name, let type, let domain, _):
                        // Create or update peer
                        let peerId = "\(name).\(type).\(domain)"
                        
                        if self.peerMap[peerId] == nil {
                            // Resolve the endpoint to get hostname and port
                            self.resolveEndpoint(result.endpoint, name: name, id: peerId)
                        } else {
                            // Update last seen time
                            self.peerMap[peerId]?.lastSeen = Date()
                        }
                    default:
                        break
                    }
                }
                
                self.updatePeerList()
            }
        }
        
        browser.start(queue: .main)
    }
    
    func stopDiscovery() {
        browser?.cancel()
        browser = nil
    }
    
    private func resolveEndpoint(_ endpoint: NWEndpoint, name: String, id: String) {
        // Create a connection to resolve the endpoint
        let connection = NWConnection(to: endpoint, using: .tcp)
        
        connection.stateUpdateHandler = { [weak self] state in
            Task { @MainActor in
                guard let self = self else { return }
                
                switch state {
                case .ready:
                    if let remoteEndpoint = connection.currentPath?.remoteEndpoint {
                        self.addResolvedPeer(id: id, name: name, endpoint: remoteEndpoint)
                    }
                    connection.cancel()
                case .failed(_), .cancelled:
                    connection.cancel()
                default:
                    break
                }
            }
        }
        
        connection.start(queue: .main)
    }
    
    private func addResolvedPeer(id: String, name: String, endpoint: NWEndpoint) {
        var hostname = "unknown"
        var port: UInt16 = 8000
        
        switch endpoint {
        case .hostPort(let host, let p):
            switch host {
            case .name(let h, _):
                hostname = h
            case .ipv4(let addr):
                hostname = addr.debugDescription
            case .ipv6(let addr):
                hostname = addr.debugDescription
            @unknown default:
                hostname = "unknown"
            }
            port = p.rawValue
        default:
            break
        }
        
        let peer = Peer(id: id, name: name, hostname: hostname, port: port)
        peerMap[id] = peer
        updatePeerList()
    }
    
    private func updatePeerList() {
        // Remove stale peers (not seen in last 60 seconds)
        let now = Date()
        peerMap = peerMap.filter { _, peer in
            guard let lastSeen = peer.lastSeen else { return true }
            return now.timeIntervalSince(lastSeen) < 60
        }
        
        discoveredPeers = Array(peerMap.values).sorted { $0.name < $1.name }
    }
}
