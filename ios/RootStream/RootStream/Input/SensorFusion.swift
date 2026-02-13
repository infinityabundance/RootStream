//
//  SensorFusion.swift
//  RootStream iOS
//
//  Sensor fusion for gyroscope and accelerometer data
//

import Foundation
import CoreMotion

class SensorFusion: ObservableObject {
    @Published var gyroData: CMRotationRate?
    @Published var accelerometerData: CMAcceleration?
    @Published var deviceMotion: CMDeviceMotion?
    
    private let motionManager = CMMotionManager()
    private let updateInterval: TimeInterval = 1.0 / 60.0 // 60 Hz
    
    private var isActive = false
    
    init() {
        setupMotionManager()
    }
    
    // MARK: - Setup
    private func setupMotionManager() {
        motionManager.gyroUpdateInterval = updateInterval
        motionManager.accelerometerUpdateInterval = updateInterval
        motionManager.deviceMotionUpdateInterval = updateInterval
    }
    
    // MARK: - Start/Stop
    func startSensorUpdates() {
        guard !isActive else { return }
        isActive = true
        
        // Start gyroscope
        if motionManager.isGyroAvailable {
            motionManager.startGyroUpdates(to: .main) { [weak self] data, error in
                guard let data = data else { return }
                self?.gyroData = data.rotationRate
            }
        }
        
        // Start accelerometer
        if motionManager.isAccelerometerAvailable {
            motionManager.startAccelerometerUpdates(to: .main) { [weak self] data, error in
                guard let data = data else { return }
                self?.accelerometerData = data.acceleration
            }
        }
        
        // Start device motion (includes sensor fusion)
        if motionManager.isDeviceMotionAvailable {
            motionManager.startDeviceMotionUpdates(to: .main) { [weak self] motion, error in
                guard let motion = motion else { return }
                self?.deviceMotion = motion
            }
        }
    }
    
    func stopSensorUpdates() {
        guard isActive else { return }
        isActive = false
        
        motionManager.stopGyroUpdates()
        motionManager.stopAccelerometerUpdates()
        motionManager.stopDeviceMotionUpdates()
    }
    
    // MARK: - Data Access
    func getOrientation() -> (roll: Double, pitch: Double, yaw: Double)? {
        guard let motion = deviceMotion else { return nil }
        
        let attitude = motion.attitude
        return (roll: attitude.roll, pitch: attitude.pitch, yaw: attitude.yaw)
    }
    
    func getGravity() -> CMAcceleration? {
        return deviceMotion?.gravity
    }
    
    func getUserAcceleration() -> CMAcceleration? {
        return deviceMotion?.userAcceleration
    }
    
    func getRotationRate() -> CMRotationRate? {
        return deviceMotion?.rotationRate
    }
    
    // MARK: - Motion Events
    func detectShake() -> Bool {
        guard let accel = accelerometerData else { return false }
        
        let magnitude = sqrt(pow(accel.x, 2) + pow(accel.y, 2) + pow(accel.z, 2))
        return magnitude > 2.5 // Threshold for shake detection
    }
    
    func getMotionVector() -> (x: Double, y: Double, z: Double)? {
        guard let motion = deviceMotion else { return nil }
        
        let accel = motion.userAcceleration
        return (x: accel.x, y: accel.y, z: accel.z)
    }
}
