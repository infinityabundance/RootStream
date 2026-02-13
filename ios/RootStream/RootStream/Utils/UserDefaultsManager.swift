//
//  UserDefaultsManager.swift
//  RootStream iOS
//
//  Persistent storage for user preferences and settings
//

import Foundation

class UserDefaultsManager {
    private let defaults = UserDefaults.standard
    
    // MARK: - Keys
    private enum Keys {
        static let videoCodec = "videoCodec"
        static let videoBitrate = "videoBitrate"
        static let videoResolution = "videoResolution"
        static let targetFPS = "targetFPS"
        static let audioEnabled = "audioEnabled"
        static let hapticFeedbackEnabled = "hapticFeedbackEnabled"
        static let showOnScreenControls = "showOnScreenControls"
        static let batteryOptimizationEnabled = "batteryOptimizationEnabled"
        static let biometricAuthEnabled = "biometricAuthEnabled"
    }
    
    // MARK: - Settings
    struct Settings {
        var videoCodec: VideoCodec = .h264
        var videoBitrate: Int = 10_000_000 // 10 Mbps
        var videoResolution: Resolution = .hd1080p
        var targetFPS: Int = 60
        var audioEnabled: Bool = true
        var hapticFeedbackEnabled: Bool = true
        var showOnScreenControls: Bool = true
        var batteryOptimizationEnabled: Bool = true
        var biometricAuthEnabled: Bool = true
    }
    
    // MARK: - Save Settings
    func saveSettings(_ settings: Settings) {
        defaults.set(settings.videoCodec.rawValue, forKey: Keys.videoCodec)
        defaults.set(settings.videoBitrate, forKey: Keys.videoBitrate)
        defaults.set(settings.videoResolution.rawValue, forKey: Keys.videoResolution)
        defaults.set(settings.targetFPS, forKey: Keys.targetFPS)
        defaults.set(settings.audioEnabled, forKey: Keys.audioEnabled)
        defaults.set(settings.hapticFeedbackEnabled, forKey: Keys.hapticFeedbackEnabled)
        defaults.set(settings.showOnScreenControls, forKey: Keys.showOnScreenControls)
        defaults.set(settings.batteryOptimizationEnabled, forKey: Keys.batteryOptimizationEnabled)
        defaults.set(settings.biometricAuthEnabled, forKey: Keys.biometricAuthEnabled)
    }
    
    // MARK: - Load Settings
    func loadSettings() -> Settings {
        var settings = Settings()
        
        if let codecString = defaults.string(forKey: Keys.videoCodec),
           let codec = VideoCodec(rawValue: codecString) {
            settings.videoCodec = codec
        }
        
        let bitrate = defaults.integer(forKey: Keys.videoBitrate)
        if bitrate > 0 {
            settings.videoBitrate = bitrate
        }
        
        if let resolutionString = defaults.string(forKey: Keys.videoResolution),
           let resolution = Resolution(rawValue: resolutionString) {
            settings.videoResolution = resolution
        }
        
        let fps = defaults.integer(forKey: Keys.targetFPS)
        if fps > 0 {
            settings.targetFPS = fps
        }
        
        settings.audioEnabled = defaults.bool(forKey: Keys.audioEnabled)
        settings.hapticFeedbackEnabled = defaults.bool(forKey: Keys.hapticFeedbackEnabled)
        settings.showOnScreenControls = defaults.bool(forKey: Keys.showOnScreenControls)
        settings.batteryOptimizationEnabled = defaults.bool(forKey: Keys.batteryOptimizationEnabled)
        settings.biometricAuthEnabled = defaults.bool(forKey: Keys.biometricAuthEnabled)
        
        return settings
    }
    
    // MARK: - Clear Settings
    func clearSettings() {
        defaults.removeObject(forKey: Keys.videoCodec)
        defaults.removeObject(forKey: Keys.videoBitrate)
        defaults.removeObject(forKey: Keys.videoResolution)
        defaults.removeObject(forKey: Keys.targetFPS)
        defaults.removeObject(forKey: Keys.audioEnabled)
        defaults.removeObject(forKey: Keys.hapticFeedbackEnabled)
        defaults.removeObject(forKey: Keys.showOnScreenControls)
        defaults.removeObject(forKey: Keys.batteryOptimizationEnabled)
        defaults.removeObject(forKey: Keys.biometricAuthEnabled)
    }
}

// MARK: - Supporting Types
enum VideoCodec: String, CaseIterable {
    case h264 = "H.264"
    case h265 = "H.265"
    case vp9 = "VP9"
}

enum Resolution: String, CaseIterable {
    case hd720p = "1280x720"
    case hd1080p = "1920x1080"
    case uhd4k = "3840x2160"
    
    var dimensions: (width: Int, height: Int) {
        switch self {
        case .hd720p: return (1280, 720)
        case .hd1080p: return (1920, 1080)
        case .uhd4k: return (3840, 2160)
        }
    }
}
