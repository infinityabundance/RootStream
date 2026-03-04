// ClipboardManager.swift - Clipboard sync over encrypted side-channel
import Foundation
import UIKit

// MARK: - Delegate protocol

protocol ClipboardManagerDelegate: AnyObject {
    func onClipboardChanged(_ content: String)
}

// MARK: - ClipboardManager

class ClipboardManager {

    static let shared = ClipboardManager()

    weak var delegate: ClipboardManagerDelegate?

    private var syncTimer: Timer?
    private var lastHash: Int = 0

    private init() {}

    // MARK: - Public API

    func startSync() {
        guard syncTimer == nil else { return }
        syncTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.syncToRemote()
        }
    }

    func stopSync() {
        syncTimer?.invalidate()
        syncTimer = nil
    }

    func setRemoteContent(_ content: String) {
        UIPasteboard.general.string = content
        lastHash = content.hashValue
    }

    func getLocalContent() -> String? {
        guard UIPasteboard.general.hasStrings else { return nil }
        return UIPasteboard.general.string
    }

    // MARK: - Private

    private func syncToRemote() {
        guard UIPasteboard.general.hasStrings,
              let content = UIPasteboard.general.string else { return }

        let hash = content.hashValue
        guard hash != lastHash else { return }
        lastHash = hash
        delegate?.onClipboardChanged(content)
    }
}
