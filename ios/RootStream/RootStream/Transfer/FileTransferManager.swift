// FileTransferManager.swift - File transfer via UIDocumentPickerViewController
import Foundation
import UIKit
import UniformTypeIdentifiers

// MARK: - Delegate protocol

protocol FileTransferDelegate: AnyObject {
    func onProgress(filename: String, sent: Int64, total: Int64)
    func onComplete(filename: String)
    func onError(filename: String, error: String)
}

// MARK: - FileTransferManager

class FileTransferManager: NSObject, UIDocumentPickerDelegate {

    static let DATA_TRANSFER_PACKET_TYPE: UInt8 = 0x08

    private static let chunkSize: Int = 64 * 1024
    private static let packetMagic: [UInt8] = [0x52, 0x53]

    weak var delegate: FileTransferDelegate?

    private var activeTransfers: [String: Bool] = [:]

    // MARK: - File picker

    func presentFilePicker(from viewController: UIViewController) {
        let picker: UIDocumentPickerViewController
        if #available(iOS 14.0, *) {
            picker = UIDocumentPickerViewController(forOpeningContentTypes: [.data, .item],
                                                   asCopy: true)
        } else {
            picker = UIDocumentPickerViewController(documentTypes: ["public.data", "public.item"],
                                                    in: .import)
        }
        picker.delegate = self
        picker.allowsMultipleSelection = false
        viewController.present(picker, animated: true)
    }

    // MARK: - Send

    func sendFile(at url: URL) {
        let filename = url.lastPathComponent
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            guard let self = self else { return }
            do {
                let data = try Data(contentsOf: url)
                let total = Int64(data.count)
                var offset = 0

                self.activeTransfers[filename] = true

                while offset < data.count {
                    guard self.activeTransfers[filename] == true else { break }

                    let end = min(offset + FileTransferManager.chunkSize, data.count)
                    let chunk = data[offset..<end]
                    let packet = self.buildPacket(type: FileTransferManager.DATA_TRANSFER_PACKET_TYPE,
                                                 payload: Array(chunk))
                    _ = packet  // TODO: write packet to streaming connection

                    offset = end
                    let sent = Int64(offset)
                    DispatchQueue.main.async {
                        self.delegate?.onProgress(filename: filename, sent: sent, total: total)
                    }
                }

                self.activeTransfers.removeValue(forKey: filename)
                DispatchQueue.main.async {
                    self.delegate?.onComplete(filename: filename)
                }
            } catch {
                DispatchQueue.main.async {
                    self.delegate?.onError(filename: filename, error: error.localizedDescription)
                }
            }
        }
    }

    // MARK: - Receive

    func receiveFile(filename: String, totalSize: Int64) {
        activeTransfers[filename] = true
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            guard let self = self else { return }
            // TODO: read incoming DATA_TRANSFER chunks from streaming connection
            // and reassemble into a file in the caches directory.
            let cacheURL = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask)[0]
                .appendingPathComponent(filename)
            _ = cacheURL

            self.activeTransfers.removeValue(forKey: filename)
            DispatchQueue.main.async {
                self.delegate?.onComplete(filename: filename)
            }
        }
    }

    // MARK: - Cancel

    func cancelTransfer(filename: String) {
        activeTransfers[filename] = false
        activeTransfers.removeValue(forKey: filename)
    }

    // MARK: - Packet builder

    private func buildPacket(type: UInt8, payload: [UInt8]) -> [UInt8] {
        let header: [UInt8] = [
            FileTransferManager.packetMagic[0], // 0x52
            FileTransferManager.packetMagic[1], // 0x53
            type,
            0x00  // reserved
        ]
        return header + payload
    }

    // MARK: - UIDocumentPickerDelegate

    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
        guard let url = urls.first else { return }
        sendFile(at: url)
    }

    func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
        // No action required
    }
}
