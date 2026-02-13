//
//  AppState.swift
//  RootStream iOS
//
//  Global application state management
//

import SwiftUI
import Combine

@MainActor
class AppState: ObservableObject {
    static let shared = AppState()
    
    // MARK: - Published Properties
    @Published var isAuthenticated = false
    @Published var currentUser: String?
    @Published var isConnected = false
    @Published var selectedPeer: Peer?
    @Published var connectionStatus: ConnectionStatus = .disconnected
    @Published var errorMessage: String?
    
    // MARK: - Services
    private let keychainManager: KeychainManager
    private let userDefaultsManager: UserDefaultsManager
    private let securityManager: SecurityManager
    
    // MARK: - Initialization
    private init() {
        self.keychainManager = KeychainManager()
        self.userDefaultsManager = UserDefaultsManager()
        self.securityManager = SecurityManager()
    }
    
    func initialize() {
        // Check for stored credentials
        loadStoredCredentials()
        
        // Load user preferences
        loadUserPreferences()
    }
    
    // MARK: - Authentication
    func login(username: String, password: String) async throws {
        // Authenticate with security manager
        let success = try await securityManager.authenticate(username: username, password: password)
        
        if success {
            self.isAuthenticated = true
            self.currentUser = username
            
            // Store credentials securely
            try keychainManager.store(username: username, password: password)
        } else {
            throw AppError.authenticationFailed
        }
    }
    
    func logout() {
        self.isAuthenticated = false
        self.currentUser = nil
        self.isConnected = false
        self.selectedPeer = nil
        self.connectionStatus = .disconnected
    }
    
    // MARK: - Private Methods
    private func loadStoredCredentials() {
        if let credentials = keychainManager.loadCredentials() {
            self.currentUser = credentials.username
            // Auto-login would require password verification
        }
    }
    
    private func loadUserPreferences() {
        // Load preferences from UserDefaults
        _ = userDefaultsManager.loadSettings()
    }
}

// MARK: - Supporting Types
enum ConnectionStatus {
    case disconnected
    case connecting
    case connected
    case error(String)
    
    var description: String {
        switch self {
        case .disconnected: return "Disconnected"
        case .connecting: return "Connecting..."
        case .connected: return "Connected"
        case .error(let msg): return "Error: \(msg)"
        }
    }
}

enum AppError: LocalizedError {
    case authenticationFailed
    case networkError
    case decodingError
    
    var errorDescription: String? {
        switch self {
        case .authenticationFailed: return "Authentication failed"
        case .networkError: return "Network error occurred"
        case .decodingError: return "Failed to decode data"
        }
    }
}
