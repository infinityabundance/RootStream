//
//  RootStreamApp.swift
//  RootStream iOS
//
//  Main app entry point for RootStream iOS application
//

import SwiftUI

@main
struct RootStreamApp: App {
    @StateObject private var appState = AppState.shared
    
    init() {
        // Initialize global configurations
        setupAppearance()
        setupNetworking()
    }
    
    var body: some Scene {
        WindowGroup {
            MainTabView()
                .environmentObject(appState)
                .onAppear {
                    appState.initialize()
                }
        }
    }
    
    private func setupAppearance() {
        // Configure app-wide appearance
        UINavigationBar.appearance().largeTitleTextAttributes = [.foregroundColor: UIColor.systemBlue]
    }
    
    private func setupNetworking() {
        // Configure networking parameters
        URLCache.shared.memoryCapacity = 10 * 1024 * 1024 // 10 MB
        URLCache.shared.diskCapacity = 50 * 1024 * 1024 // 50 MB
    }
}
