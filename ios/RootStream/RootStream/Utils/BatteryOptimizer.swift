//
//  BatteryOptimizer.swift
//  RootStream iOS
//
//  Battery optimization and thermal management
//

import Foundation
import UIKit

class BatteryOptimizer: ObservableObject {
    @Published var batteryLevel: Float = 1.0
    @Published var batteryState: UIDevice.BatteryState = .unknown
    @Published var isLowPowerModeEnabled: Bool = false
    @Published var thermalState: ProcessInfo.ThermalState = .nominal
    
    @Published var recommendedFPS: Int = 60
    @Published var recommendedResolution: Resolution = .hd1080p
    
    private var batteryMonitor: Timer?
    
    init() {
        UIDevice.current.isBatteryMonitoringEnabled = true
        startMonitoring()
    }
    
    deinit {
        stopMonitoring()
    }
    
    // MARK: - Monitoring
    func startMonitoring() {
        updateBatteryState()
        updateThermalState()
        
        // Monitor battery changes
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(batteryStateChanged),
            name: UIDevice.batteryStateDidChangeNotification,
            object: nil
        )
        
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(batteryLevelChanged),
            name: UIDevice.batteryLevelDidChangeNotification,
            object: nil
        )
        
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(lowPowerModeChanged),
            name: .NSProcessInfoPowerStateDidChange,
            object: nil
        )
        
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(thermalStateChanged),
            name: ProcessInfo.thermalStateDidChangeNotification,
            object: nil
        )
        
        // Periodic updates
        batteryMonitor = Timer.scheduledTimer(withTimeInterval: 10.0, repeats: true) { [weak self] _ in
            self?.updateOptimizations()
        }
    }
    
    func stopMonitoring() {
        batteryMonitor?.invalidate()
        NotificationCenter.default.removeObserver(self)
    }
    
    // MARK: - Battery State Updates
    @objc private func batteryStateChanged() {
        updateBatteryState()
    }
    
    @objc private func batteryLevelChanged() {
        updateBatteryState()
    }
    
    @objc private func lowPowerModeChanged() {
        isLowPowerModeEnabled = ProcessInfo.processInfo.isLowPowerModeEnabled
        updateOptimizations()
    }
    
    @objc private func thermalStateChanged() {
        updateThermalState()
    }
    
    private func updateBatteryState() {
        batteryLevel = UIDevice.current.batteryLevel
        batteryState = UIDevice.current.batteryState
        updateOptimizations()
    }
    
    private func updateThermalState() {
        thermalState = ProcessInfo.processInfo.thermalState
        updateOptimizations()
    }
    
    // MARK: - Optimization Logic
    private func updateOptimizations() {
        // Determine recommended settings based on battery and thermal state
        
        if isLowPowerModeEnabled {
            // Aggressive power saving
            recommendedFPS = 30
            recommendedResolution = .hd720p
        } else if batteryLevel < 0.2 {
            // Low battery
            recommendedFPS = 30
            recommendedResolution = .hd720p
        } else if batteryLevel < 0.5 {
            // Medium battery
            recommendedFPS = 45
            recommendedResolution = .hd1080p
        } else {
            // High battery
            recommendedFPS = 60
            recommendedResolution = .hd1080p
        }
        
        // Thermal throttling
        switch thermalState {
        case .serious:
            recommendedFPS = min(recommendedFPS, 30)
            recommendedResolution = .hd720p
        case .critical:
            recommendedFPS = min(recommendedFPS, 24)
            recommendedResolution = .hd720p
        default:
            break
        }
    }
    
    // MARK: - Optimization Recommendations
    func shouldReduceQuality() -> Bool {
        return batteryLevel < 0.3 || isLowPowerModeEnabled || thermalState == .serious || thermalState == .critical
    }
    
    func shouldPauseBackgroundTasks() -> Bool {
        return isLowPowerModeEnabled || thermalState == .critical
    }
    
    func getOptimizationStatus() -> String {
        var status = "Battery: \(Int(batteryLevel * 100))%"
        
        if isLowPowerModeEnabled {
            status += " (Low Power Mode)"
        }
        
        switch thermalState {
        case .nominal:
            status += " | Thermal: Normal"
        case .fair:
            status += " | Thermal: Fair"
        case .serious:
            status += " | Thermal: High"
        case .critical:
            status += " | Thermal: Critical"
        @unknown default:
            break
        }
        
        return status
    }
}
