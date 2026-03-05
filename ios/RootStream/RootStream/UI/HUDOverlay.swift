// HUDOverlay.swift - Swipe-up latency/bitrate overlay
import SwiftUI

// MARK: - Stats model

struct HUDStats {
    var latencyMs: Double
    var bitrateKbps: Double
    var fps: Double
    var packetLoss: Double
}

// MARK: - HUDOverlay

struct HUDOverlay: View {

    @Binding var stats: HUDStats
    @State private var isVisible: Bool = false

    private let overlayHeight: CGFloat = 120

    var body: some View {
        GeometryReader { geometry in
            VStack(spacing: 8) {
                Capsule()
                    .frame(width: 40, height: 4)
                    .foregroundColor(.white.opacity(0.5))
                    .padding(.top, 8)

                HStack(spacing: 24) {
                    statView(label: "Latency", value: String(format: "%.0f ms", stats.latencyMs))
                    statView(label: "Bitrate", value: String(format: "%.1f Mbps", stats.bitrateKbps / 1000))
                    statView(label: "FPS",     value: String(format: "%.0f", stats.fps))
                    statView(label: "Loss",    value: String(format: "%.1f%%", stats.packetLoss))
                }
                .padding(.horizontal, 16)
                .padding(.bottom, 12)
            }
            .frame(maxWidth: .infinity)
            .frame(height: overlayHeight)
            .background(Color.black.opacity(0.72))
            .cornerRadius(16, corners: [.topLeft, .topRight])
            // Slide in from the bottom
            .offset(y: isVisible
                    ? geometry.size.height - overlayHeight
                    : geometry.size.height)
            .animation(.spring(response: 0.35, dampingFraction: 0.75), value: isVisible)
            .gesture(
                DragGesture(minimumDistance: 20)
                    .onEnded { value in
                        if value.translation.height < -20 { isVisible = true  }
                        if value.translation.height >  20 { isVisible = false }
                    }
            )
        }
        .ignoresSafeArea(edges: .bottom)
    }

    // MARK: - Helpers

    private func statView(label: String, value: String) -> some View {
        VStack(spacing: 2) {
            Text(value)
                .font(.system(size: 16, weight: .semibold, design: .monospaced))
                .foregroundColor(.white)
            Text(label)
                .font(.system(size: 10, weight: .regular))
                .foregroundColor(.white.opacity(0.7))
        }
    }
}

// MARK: - Rounded corner helper

private extension View {
    func cornerRadius(_ radius: CGFloat, corners: UIRectCorner) -> some View {
        clipShape(RoundedCorner(radius: radius, corners: corners))
    }
}

private struct RoundedCorner: Shape {
    var radius: CGFloat
    var corners: UIRectCorner

    func path(in rect: CGRect) -> Path {
        let path = UIBezierPath(roundedRect: rect,
                                byRoundingCorners: corners,
                                cornerRadii: CGSize(width: radius, height: radius))
        return Path(path.cgPath)
    }
}
