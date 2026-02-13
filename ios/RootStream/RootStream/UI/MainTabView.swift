//
//  MainTabView.swift
//  RootStream iOS
//
//  Main tab-based navigation for the app
//

import SwiftUI

struct MainTabView: View {
    @EnvironmentObject var appState: AppState
    @State private var selectedTab = 0
    
    var body: some View {
        if !appState.isAuthenticated {
            LoginView()
        } else {
            TabView(selection: $selectedTab) {
                PeerDiscoveryView()
                    .tabItem {
                        Label("Discover", systemImage: "network")
                    }
                    .tag(0)
                
                StreamView()
                    .tabItem {
                        Label("Stream", systemImage: "play.rectangle.fill")
                    }
                    .tag(1)
                
                SettingsView()
                    .tabItem {
                        Label("Settings", systemImage: "gear")
                    }
                    .tag(2)
            }
        }
    }
}

#Preview {
    MainTabView()
        .environmentObject(AppState.shared)
}
