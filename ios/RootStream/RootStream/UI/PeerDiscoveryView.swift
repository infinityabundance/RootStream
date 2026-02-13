//
//  PeerDiscoveryView.swift
//  RootStream iOS
//
//  Peer discovery with mDNS and manual peer addition
//

import SwiftUI

struct PeerDiscoveryView: View {
    @EnvironmentObject var appState: AppState
    @StateObject private var peerDiscovery = PeerDiscovery()
    @State private var showAddPeerSheet = false
    @State private var manualAddress = ""
    @State private var manualPort = "8000"
    
    var body: some View {
        NavigationView {
            List {
                Section(header: Text("Discovered Peers")) {
                    if peerDiscovery.discoveredPeers.isEmpty {
                        HStack {
                            ProgressView()
                            Text("Searching for peers...")
                                .foregroundColor(.secondary)
                        }
                    } else {
                        ForEach(peerDiscovery.discoveredPeers) { peer in
                            PeerRow(peer: peer)
                                .onTapGesture {
                                    selectPeer(peer)
                                }
                        }
                    }
                }
                
                Section(header: Text("Manual Connection")) {
                    Button(action: { showAddPeerSheet = true }) {
                        HStack {
                            Image(systemName: "plus.circle.fill")
                            Text("Add Peer Manually")
                        }
                    }
                }
            }
            .navigationTitle("Discover Peers")
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button(action: { peerDiscovery.startDiscovery() }) {
                        Image(systemName: "arrow.clockwise")
                    }
                }
            }
            .sheet(isPresented: $showAddPeerSheet) {
                AddPeerSheet(address: $manualAddress, port: $manualPort) {
                    addManualPeer()
                }
            }
            .onAppear {
                peerDiscovery.startDiscovery()
            }
            .onDisappear {
                peerDiscovery.stopDiscovery()
            }
        }
    }
    
    private func selectPeer(_ peer: Peer) {
        appState.selectedPeer = peer
        appState.connectionStatus = .connecting
    }
    
    private func addManualPeer() {
        guard let port = UInt16(manualPort) else { return }
        
        let manualPeer = Peer(
            id: UUID().uuidString,
            name: manualAddress,
            hostname: manualAddress,
            port: port,
            isManual: true
        )
        
        selectPeer(manualPeer)
        showAddPeerSheet = false
    }
}

struct PeerRow: View {
    let peer: Peer
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(peer.name)
                .font(.headline)
            
            Text("\(peer.hostname):\(peer.port)")
                .font(.caption)
                .foregroundColor(.secondary)
            
            if let lastSeen = peer.lastSeen {
                Text("Last seen: \(lastSeen, style: .relative)")
                    .font(.caption2)
                    .foregroundColor(.secondary)
            }
        }
        .padding(.vertical, 4)
    }
}

struct AddPeerSheet: View {
    @Binding var address: String
    @Binding var port: String
    @Environment(\.dismiss) var dismiss
    let onAdd: () -> Void
    
    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Peer Information")) {
                    TextField("Hostname or IP", text: $address)
                        .textContentType(.URL)
                        .autocapitalization(.none)
                    
                    TextField("Port", text: $port)
                        .keyboardType(.numberPad)
                }
            }
            .navigationTitle("Add Peer")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        dismiss()
                    }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Add") {
                        onAdd()
                        dismiss()
                    }
                    .disabled(address.isEmpty || port.isEmpty)
                }
            }
        }
    }
}

#Preview {
    PeerDiscoveryView()
        .environmentObject(AppState.shared)
}
