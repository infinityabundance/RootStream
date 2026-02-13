//
//  InputController.swift
//  RootStream iOS
//
//  Input management for touch, gamepad, and sensors
//

import Foundation
import GameController
import CoreHaptics

@MainActor
class InputController: ObservableObject {
    @Published var joystickPosition: CGPoint = .zero
    @Published var buttonStates: [String: Bool] = [:]
    
    private var hapticEngine: CHHapticEngine?
    private var connectedGamepad: GCController?
    
    init() {
        setupHaptics()
        setupGamepadSupport()
    }
    
    // MARK: - Haptics
    private func setupHaptics() {
        guard CHHapticEngine.capabilitiesForHardware().supportsHaptics else {
            return
        }
        
        do {
            hapticEngine = try CHHapticEngine()
            try hapticEngine?.start()
        } catch {
            print("Failed to start haptic engine: \(error)")
        }
    }
    
    func triggerHapticFeedback(intensity: Float = 0.5, sharpness: Float = 0.5) {
        guard let engine = hapticEngine else { return }
        
        let intensity = CHHapticEventParameter(parameterID: .hapticIntensity, value: intensity)
        let sharpness = CHHapticEventParameter(parameterID: .hapticSharpness, value: sharpness)
        
        let event = CHHapticEvent(
            eventType: .hapticTransient,
            parameters: [intensity, sharpness],
            relativeTime: 0
        )
        
        do {
            let pattern = try CHHapticPattern(events: [event], parameters: [])
            let player = try engine.makePlayer(with: pattern)
            try player.start(atTime: CHHapticTimeImmediate)
        } catch {
            print("Failed to play haptic: \(error)")
        }
    }
    
    // MARK: - Gamepad Support
    private func setupGamepadSupport() {
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(gamepadConnected),
            name: .GCControllerDidConnect,
            object: nil
        )
        
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(gamepadDisconnected),
            name: .GCControllerDidDisconnect,
            object: nil
        )
        
        // Check for already connected gamepads
        if let gamepad = GCController.controllers().first {
            connectGamepad(gamepad)
        }
    }
    
    @objc private func gamepadConnected(_ notification: Notification) {
        guard let gamepad = notification.object as? GCController else { return }
        connectGamepad(gamepad)
    }
    
    @objc private func gamepadDisconnected(_ notification: Notification) {
        connectedGamepad = nil
    }
    
    private func connectGamepad(_ gamepad: GCController) {
        connectedGamepad = gamepad
        
        // Setup button handlers
        if let extendedGamepad = gamepad.extendedGamepad {
            extendedGamepad.buttonA.valueChangedHandler = { [weak self] _, _, pressed in
                self?.handleButtonPress("A", pressed: pressed)
            }
            
            extendedGamepad.buttonB.valueChangedHandler = { [weak self] _, _, pressed in
                self?.handleButtonPress("B", pressed: pressed)
            }
            
            extendedGamepad.buttonX.valueChangedHandler = { [weak self] _, _, pressed in
                self?.handleButtonPress("X", pressed: pressed)
            }
            
            extendedGamepad.buttonY.valueChangedHandler = { [weak self] _, _, pressed in
                self?.handleButtonPress("Y", pressed: pressed)
            }
            
            // Left stick
            extendedGamepad.leftThumbstick.valueChangedHandler = { [weak self] _, xValue, yValue in
                self?.handleJoystickMove(x: CGFloat(xValue), y: CGFloat(yValue))
            }
        }
    }
    
    // MARK: - Input Handling
    func handleButtonPress(_ button: String, pressed: Bool) {
        buttonStates[button] = pressed
        
        if pressed {
            triggerHapticFeedback()
        }
        
        // Send input event to server
        // This would be handled by StreamingClient
    }
    
    func handleJoystickMove(x: CGFloat, y: CGFloat) {
        joystickPosition = CGPoint(x: x, y: y)
        
        // Send input event to server
        // This would be handled by StreamingClient
    }
    
    func handleTouchInput(location: CGPoint, phase: UITouch.Phase) {
        // Handle touch input
        // This would be converted to mouse events or touch events for the server
    }
}
