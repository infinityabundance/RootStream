//
//  KeychainManager.swift
//  RootStream iOS
//
//  Secure storage for credentials and sensitive data using Keychain
//

import Foundation
import Security

class KeychainManager {
    private let service = "com.rootstream.ios"
    
    // MARK: - Store Credentials
    func store(username: String, password: String) throws {
        let passwordData = password.data(using: .utf8)!
        
        // Create query
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: username,
            kSecValueData as String: passwordData
        ]
        
        // Delete any existing item
        SecItemDelete(query as CFDictionary)
        
        // Add new item
        let status = SecItemAdd(query as CFDictionary, nil)
        
        guard status == errSecSuccess else {
            throw KeychainError.storeFailed
        }
    }
    
    // MARK: - Load Credentials
    func loadCredentials() -> (username: String, password: String)? {
        // For security, we only load username, not password
        // Password should be re-entered by user
        if let username = loadUsername() {
            return (username, "")
        }
        return nil
    }
    
    private func loadUsername() -> String? {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecMatchLimit as String: kSecMatchLimitOne,
            kSecReturnAttributes as String: true
        ]
        
        var item: CFTypeRef?
        let status = SecItemCopyMatching(query as CFDictionary, &item)
        
        guard status == errSecSuccess,
              let existingItem = item as? [String: Any],
              let username = existingItem[kSecAttrAccount as String] as? String else {
            return nil
        }
        
        return username
    }
    
    // MARK: - Delete Credentials
    func deleteCredentials() throws {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service
        ]
        
        let status = SecItemDelete(query as CFDictionary)
        
        guard status == errSecSuccess || status == errSecItemNotFound else {
            throw KeychainError.deleteFailed
        }
    }
    
    // MARK: - Store Session Token
    func storeSessionToken(_ token: String) throws {
        let tokenData = token.data(using: .utf8)!
        
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: "session_token",
            kSecValueData as String: tokenData
        ]
        
        SecItemDelete(query as CFDictionary)
        let status = SecItemAdd(query as CFDictionary, nil)
        
        guard status == errSecSuccess else {
            throw KeychainError.storeFailed
        }
    }
    
    // MARK: - Load Session Token
    func loadSessionToken() -> String? {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: "session_token",
            kSecReturnData as String: true
        ]
        
        var item: CFTypeRef?
        let status = SecItemCopyMatching(query as CFDictionary, &item)
        
        guard status == errSecSuccess,
              let tokenData = item as? Data,
              let token = String(data: tokenData, encoding: .utf8) else {
            return nil
        }
        
        return token
    }
}

// MARK: - Errors
enum KeychainError: Error {
    case storeFailed
    case deleteFailed
    case loadFailed
}
