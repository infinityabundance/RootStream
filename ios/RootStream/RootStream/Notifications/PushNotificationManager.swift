// PushNotificationManager.swift - APNs push notifications for stream invites
import Foundation
import UserNotifications
import UIKit

class PushNotificationManager: NSObject, UNUserNotificationCenterDelegate {

    static let shared = PushNotificationManager()

    var deviceToken: String?

    private override init() {
        super.init()
        UNUserNotificationCenter.current().delegate = self
    }

    // MARK: - Registration

    func requestPermission() {
        UNUserNotificationCenter.current().requestAuthorization(options: [.alert, .sound, .badge]) { granted, error in
            if let error = error {
                print("PushNotificationManager: permission error \(error)")
            }
        }
    }

    func registerForRemoteNotifications() {
        DispatchQueue.main.async {
            UIApplication.shared.registerForRemoteNotifications()
        }
    }

    func didRegisterWithToken(_ deviceToken: Data) {
        let hex = deviceToken.map { String(format: "%02x", $0) }.joined()
        self.deviceToken = hex
        // TODO: send `hex` to the RootStream server for push targeting
    }

    // MARK: - Stream invite notifications

    func handleStreamInvite(from hostName: String, hostAddress: String) {
        let content = UNMutableNotificationContent()
        content.title = "Stream Invite"
        content.body  = "\(hostName) invites you to connect"
        content.sound = .default
        content.userInfo = ["hostName": hostName, "hostAddress": hostAddress]

        let request = UNNotificationRequest(
            identifier: "invite-\(hostAddress)",
            content: content,
            trigger: nil  // deliver immediately
        )
        UNUserNotificationCenter.current().add(request) { error in
            if let error = error {
                print("PushNotificationManager: notification error \(error)")
            }
        }
    }

    func didReceiveRemoteNotification(_ userInfo: [AnyHashable: Any]) {
        guard let hostName    = userInfo["hostName"]    as? String,
              let hostAddress = userInfo["hostAddress"] as? String else { return }
        handleStreamInvite(from: hostName, hostAddress: hostAddress)
    }

    // MARK: - UNUserNotificationCenterDelegate

    func userNotificationCenter(_ center: UNUserNotificationCenter,
                                willPresent notification: UNNotification,
                                withCompletionHandler completionHandler: @escaping (UNNotificationPresentationOptions) -> Void) {
        if #available(iOS 14.0, *) {
            completionHandler([.banner, .sound])
        } else {
            completionHandler([.alert, .sound])
        }
    }

    func userNotificationCenter(_ center: UNUserNotificationCenter,
                                didReceive response: UNNotificationResponse,
                                withCompletionHandler completionHandler: @escaping () -> Void) {
        let userInfo = response.notification.request.content.userInfo
        didReceiveRemoteNotification(userInfo)
        completionHandler()
    }
}
