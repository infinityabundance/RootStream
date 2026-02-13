//
//  LoginView.swift
//  RootStream iOS
//
//  Authentication UI with username/password and biometric auth
//

import SwiftUI
import LocalAuthentication

struct LoginView: View {
    @EnvironmentObject var appState: AppState
    @State private var username = ""
    @State private var password = ""
    @State private var isLoading = false
    @State private var showError = false
    @State private var errorMessage = ""
    @State private var biometricType: LABiometryType = .none
    
    var body: some View {
        NavigationView {
            VStack(spacing: 20) {
                // Logo
                Image(systemName: "gamecontroller.fill")
                    .resizable()
                    .aspectRatio(contentMode: .fit)
                    .frame(width: 100, height: 100)
                    .foregroundColor(.blue)
                
                Text("RootStream")
                    .font(.largeTitle)
                    .fontWeight(.bold)
                
                // Username field
                TextField("Username", text: $username)
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .textContentType(.username)
                    .autocapitalization(.none)
                    .padding(.horizontal)
                
                // Password field
                SecureField("Password", text: $password)
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .textContentType(.password)
                    .padding(.horizontal)
                
                // Login button
                Button(action: login) {
                    if isLoading {
                        ProgressView()
                            .progressViewStyle(CircularProgressViewStyle(tint: .white))
                    } else {
                        Text("Login")
                            .fontWeight(.semibold)
                    }
                }
                .frame(maxWidth: .infinity)
                .padding()
                .background(Color.blue)
                .foregroundColor(.white)
                .cornerRadius(10)
                .padding(.horizontal)
                .disabled(isLoading || username.isEmpty || password.isEmpty)
                
                // Biometric authentication
                if biometricType != .none {
                    Button(action: authenticateWithBiometrics) {
                        HStack {
                            Image(systemName: biometricType == .faceID ? "faceid" : "touchid")
                            Text("Use \(biometricType == .faceID ? "Face ID" : "Touch ID")")
                        }
                    }
                    .padding()
                }
                
                Spacer()
            }
            .padding()
            .navigationTitle("Login")
            .alert("Error", isPresented: $showError) {
                Button("OK", role: .cancel) { }
            } message: {
                Text(errorMessage)
            }
            .onAppear {
                checkBiometricAvailability()
            }
        }
    }
    
    private func login() {
        isLoading = true
        
        Task {
            do {
                try await appState.login(username: username, password: password)
                isLoading = false
            } catch {
                isLoading = false
                errorMessage = error.localizedDescription
                showError = true
            }
        }
    }
    
    private func authenticateWithBiometrics() {
        let context = LAContext()
        var error: NSError?
        
        guard context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: &error) else {
            errorMessage = error?.localizedDescription ?? "Biometric authentication not available"
            showError = true
            return
        }
        
        context.evaluatePolicy(.deviceOwnerAuthenticationWithBiometrics,
                              localizedReason: "Authenticate to access RootStream") { success, error in
            DispatchQueue.main.async {
                if success {
                    // Load stored credentials and authenticate
                    // In a real app, you would securely retrieve and validate the credentials
                    Task {
                        // Simulated biometric auth - in production, integrate with secure storage
                        self.errorMessage = "Biometric auth successful - implement credential retrieval"
                        self.showError = true
                    }
                } else {
                    errorMessage = error?.localizedDescription ?? "Authentication failed"
                    showError = true
                }
            }
        }
    }
    
    private func checkBiometricAvailability() {
        let context = LAContext()
        var error: NSError?
        
        if context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: &error) {
            biometricType = context.biometryType
        }
    }
}

#Preview {
    LoginView()
        .environmentObject(AppState.shared)
}
