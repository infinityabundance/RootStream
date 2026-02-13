//
//  OnScreenJoystick.swift
//  RootStream iOS
//
//  On-screen joystick SwiftUI component
//

import SwiftUI

struct OnScreenJoystick: View {
    @ObservedObject var inputController: InputController
    @State private var dragOffset: CGSize = .zero
    
    private let baseSize: CGFloat = 100
    private let stickSize: CGFloat = 40
    private let maxOffset: CGFloat = 30
    
    var body: some View {
        ZStack {
            // Base circle
            Circle()
                .fill(Color.white.opacity(0.3))
                .frame(width: baseSize, height: baseSize)
            
            // Stick circle
            Circle()
                .fill(Color.white.opacity(0.7))
                .frame(width: stickSize, height: stickSize)
                .offset(x: dragOffset.width, y: dragOffset.height)
        }
        .gesture(
            DragGesture()
                .onChanged { value in
                    let translation = value.translation
                    let distance = sqrt(pow(translation.width, 2) + pow(translation.height, 2))
                    
                    if distance > maxOffset {
                        let angle = atan2(translation.height, translation.width)
                        dragOffset = CGSize(
                            width: cos(angle) * maxOffset,
                            height: sin(angle) * maxOffset
                        )
                    } else {
                        dragOffset = translation
                    }
                    
                    // Normalize to -1...1 range
                    let normalizedX = dragOffset.width / maxOffset
                    let normalizedY = -dragOffset.height / maxOffset // Invert Y
                    
                    inputController.handleJoystickMove(x: normalizedX, y: normalizedY)
                }
                .onEnded { _ in
                    withAnimation(.spring()) {
                        dragOffset = .zero
                    }
                    inputController.handleJoystickMove(x: 0, y: 0)
                }
        )
    }
}

struct DPadView: View {
    @ObservedObject var inputController: InputController
    
    var body: some View {
        VStack(spacing: 0) {
            // Up
            DPadButton(direction: "up") {
                inputController.handleButtonPress("DPad-Up", pressed: true)
            }
            
            HStack(spacing: 0) {
                // Left
                DPadButton(direction: "left") {
                    inputController.handleButtonPress("DPad-Left", pressed: true)
                }
                
                // Center (spacer)
                Color.clear
                    .frame(width: 40, height: 40)
                
                // Right
                DPadButton(direction: "right") {
                    inputController.handleButtonPress("DPad-Right", pressed: true)
                }
            }
            
            // Down
            DPadButton(direction: "down") {
                inputController.handleButtonPress("DPad-Down", pressed: true)
            }
        }
    }
}

struct DPadButton: View {
    let direction: String
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            Image(systemName: arrowIcon)
                .font(.title)
                .foregroundColor(.white)
                .frame(width: 40, height: 40)
                .background(Color.white.opacity(0.3))
        }
    }
    
    private var arrowIcon: String {
        switch direction {
        case "up": return "arrow.up"
        case "down": return "arrow.down"
        case "left": return "arrow.left"
        case "right": return "arrow.right"
        default: return "arrow.up"
        }
    }
}

struct ActionButtonsView: View {
    @ObservedObject var inputController: InputController
    
    var body: some View {
        ZStack {
            // Y (top)
            ActionButton(label: "Y", color: .blue)
                .offset(x: 0, y: -40)
                .onTapGesture {
                    inputController.handleButtonPress("Y", pressed: true)
                }
            
            // B (right)
            ActionButton(label: "B", color: .red)
                .offset(x: 40, y: 0)
                .onTapGesture {
                    inputController.handleButtonPress("B", pressed: true)
                }
            
            // A (bottom)
            ActionButton(label: "A", color: .green)
                .offset(x: 0, y: 40)
                .onTapGesture {
                    inputController.handleButtonPress("A", pressed: true)
                }
            
            // X (left)
            ActionButton(label: "X", color: .yellow)
                .offset(x: -40, y: 0)
                .onTapGesture {
                    inputController.handleButtonPress("X", pressed: true)
                }
        }
    }
}

struct ActionButton: View {
    let label: String
    let color: Color
    
    var body: some View {
        ZStack {
            Circle()
                .fill(color.opacity(0.7))
                .frame(width: 35, height: 35)
            
            Text(label)
                .font(.headline)
                .foregroundColor(.white)
        }
    }
}
