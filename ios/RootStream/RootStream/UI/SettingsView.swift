//
//  SettingsView.swift
//  RootStream iOS
//
//  Configuration and settings for the app
//

import SwiftUI

struct SettingsView: View {
    @EnvironmentObject var appState: AppState
    @StateObject private var settingsManager = SettingsViewModel()
    
    var body: some View {
        NavigationView {
            Form {
                // Video Settings
                Section(header: Text("Video")) {
                    Picker("Codec", selection: $settingsManager.videoCodec) {
                        ForEach(VideoCodec.allCases, id: \.self) { codec in
                            Text(codec.rawValue).tag(codec)
                        }
                    }
                    
                    Picker("Resolution", selection: $settingsManager.videoResolution) {
                        ForEach(Resolution.allCases, id: \.self) { resolution in
                            Text(resolution.rawValue).tag(resolution)
                        }
                    }
                    
                    Stepper("Target FPS: \(settingsManager.targetFPS)",
                           value: $settingsManager.targetFPS,
                           in: 30...60,
                           step: 10)
                    
                    HStack {
                        Text("Bitrate")
                        Spacer()
                        Text("\(settingsManager.videoBitrate / 1_000_000) Mbps")
                            .foregroundColor(.secondary)
                    }
                }
                
                // Audio Settings
                Section(header: Text("Audio")) {
                    Toggle("Enable Audio", isOn: $settingsManager.audioEnabled)
                }
                
                // Input Settings
                Section(header: Text("Input")) {
                    Toggle("On-Screen Controls", isOn: $settingsManager.showOnScreenControls)
                    Toggle("Haptic Feedback", isOn: $settingsManager.hapticFeedbackEnabled)
                }
                
                // Performance Settings
                Section(header: Text("Performance")) {
                    Toggle("Battery Optimization", isOn: $settingsManager.batteryOptimizationEnabled)
                }
                
                // Security Settings
                Section(header: Text("Security")) {
                    Toggle("Biometric Authentication", isOn: $settingsManager.biometricAuthEnabled)
                }
                
                // Account
                Section(header: Text("Account")) {
                    if let username = appState.currentUser {
                        Text("Logged in as: \(username)")
                            .foregroundColor(.secondary)
                    }
                    
                    Button("Logout") {
                        appState.logout()
                    }
                    .foregroundColor(.red)
                }
                
                // About
                Section(header: Text("About")) {
                    HStack {
                        Text("Version")
                        Spacer()
                        Text("1.0.0")
                            .foregroundColor(.secondary)
                    }
                    
                    Link("GitHub Repository", destination: URL(string: "https://github.com/infinityabundance/RootStream")!)
                }
            }
            .navigationTitle("Settings")
        }
    }
}

// MARK: - Settings ViewModel
class SettingsViewModel: ObservableObject {
    private let manager = UserDefaultsManager()
    
    @Published var videoCodec: VideoCodec {
        didSet { saveSettings() }
    }
    @Published var videoBitrate: Int {
        didSet { saveSettings() }
    }
    @Published var videoResolution: Resolution {
        didSet { saveSettings() }
    }
    @Published var targetFPS: Int {
        didSet { saveSettings() }
    }
    @Published var audioEnabled: Bool {
        didSet { saveSettings() }
    }
    @Published var hapticFeedbackEnabled: Bool {
        didSet { saveSettings() }
    }
    @Published var showOnScreenControls: Bool {
        didSet { saveSettings() }
    }
    @Published var batteryOptimizationEnabled: Bool {
        didSet { saveSettings() }
    }
    @Published var biometricAuthEnabled: Bool {
        didSet { saveSettings() }
    }
    
    init() {
        let settings = manager.loadSettings()
        self.videoCodec = settings.videoCodec
        self.videoBitrate = settings.videoBitrate
        self.videoResolution = settings.videoResolution
        self.targetFPS = settings.targetFPS
        self.audioEnabled = settings.audioEnabled
        self.hapticFeedbackEnabled = settings.hapticFeedbackEnabled
        self.showOnScreenControls = settings.showOnScreenControls
        self.batteryOptimizationEnabled = settings.batteryOptimizationEnabled
        self.biometricAuthEnabled = settings.biometricAuthEnabled
    }
    
    private func saveSettings() {
        let settings = UserDefaultsManager.Settings(
            videoCodec: videoCodec,
            videoBitrate: videoBitrate,
            videoResolution: videoResolution,
            targetFPS: targetFPS,
            audioEnabled: audioEnabled,
            hapticFeedbackEnabled: hapticFeedbackEnabled,
            showOnScreenControls: showOnScreenControls,
            batteryOptimizationEnabled: batteryOptimizationEnabled,
            biometricAuthEnabled: biometricAuthEnabled
        )
        manager.saveSettings(settings)
    }
}

#Preview {
    SettingsView()
        .environmentObject(AppState.shared)
}
