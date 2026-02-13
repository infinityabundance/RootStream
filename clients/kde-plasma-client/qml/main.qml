import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import RootStream 1.0

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1280
    height: 720
    title: "RootStream KDE Client"
    
    property bool isConnected: rootStreamClient.connected
    property bool isFullscreen: visibility === Window.FullScreen
    
    // Main content area
    StackLayout {
        id: contentStack
        anchors.fill: parent
        currentIndex: isConnected ? 1 : 0
        
        // Peer Selection View (when not connected)
        PeerSelectionView {
            id: peerSelectionView
        }
        
        // Stream View (when connected)
        StreamView {
            id: streamView
        }
    }
    
    // Status Bar
    StatusBar {
        id: statusBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        visible: isConnected && !isFullscreen
    }
    
    // Menu Bar
    menuBar: MenuBar {
        visible: !isFullscreen
        
        Menu {
            title: "File"
            MenuItem {
                text: "Connect..."
                onTriggered: connectionDialog.open()
            }
            MenuItem {
                text: "Disconnect"
                enabled: isConnected
                onTriggered: rootStreamClient.disconnect()
            }
            MenuSeparator {}
            MenuItem {
                text: "Settings"
                onTriggered: settingsDialog.open()
            }
            MenuSeparator {}
            MenuItem {
                text: "Quit"
                onTriggered: Qt.quit()
            }
        }
        
        Menu {
            title: "View"
            MenuItem {
                text: isFullscreen ? "Exit Fullscreen" : "Fullscreen"
                onTriggered: {
                    if (isFullscreen) {
                        mainWindow.showNormal()
                    } else {
                        mainWindow.showFullScreen()
                    }
                }
            }
            MenuItem {
                text: "Toggle Status Bar"
                enabled: isConnected
                checkable: true
                checked: statusBar.visible
                onTriggered: statusBar.visible = !statusBar.visible
            }
        }
        
        Menu {
            title: "Help"
            MenuItem {
                text: "About"
                onTriggered: aboutDialog.open()
            }
            MenuItem {
                text: "Diagnostics"
                onTriggered: diagnosticsDialog.open()
            }
        }
    }
    
    // Connection Dialog
    Dialog {
        id: connectionDialog
        title: "Connect to Peer"
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        anchors.centerIn: parent
        width: 400
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            
            Label {
                text: "Enter RootStream Code or IP:Port"
            }
            
            TextField {
                id: connectionCodeField
                Layout.fillWidth: true
                placeholderText: "kXx7YqZ3...@hostname or 192.168.1.100:9876"
            }
            
            Label {
                text: "Or select from discovered peers:"
            }
            
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 200
                model: peerManager
                
                delegate: ItemDelegate {
                    width: parent ? parent.width : 0
                    text: model.hostname + " (" + (model.discovered ? "discovered" : "manual") + ")"
                    onClicked: connectionCodeField.text = model.code
                }
            }
        }
        
        onAccepted: {
            if (connectionCodeField.text.length > 0) {
                rootStreamClient.connectToPeer(connectionCodeField.text)
            }
        }
    }
    
    // Settings Dialog
    SettingsView {
        id: settingsDialog
    }
    
    // About Dialog
    Dialog {
        id: aboutDialog
        title: "About RootStream KDE Client"
        standardButtons: Dialog.Ok
        modal: true
        anchors.centerIn: parent
        
        Label {
            text: "RootStream KDE Plasma Client\nVersion 1.0.0\n\nSecure P2P Game Streaming\n\n© 2026 RootStream Project\nLicensed under MIT"
        }
    }
    
    // Diagnostics Dialog
    Dialog {
        id: diagnosticsDialog
        title: "System Diagnostics"
        standardButtons: Dialog.Close
        modal: true
        anchors.centerIn: parent
        width: 600
        height: 400
        
        ScrollView {
            anchors.fill: parent
            
            TextArea {
                text: rootStreamClient.systemDiagnostics()
                readOnly: true
                selectByMouse: true
            }
        }
    }
    
    // Keyboard shortcuts
    Shortcut {
        sequence: "F11"
        onActivated: {
            if (isFullscreen) {
                mainWindow.showNormal()
            } else {
                mainWindow.showFullScreen()
            }
        }
    }
    
    Shortcut {
        sequence: "Escape"
        enabled: isFullscreen
        onActivated: mainWindow.showNormal()
    }
    
    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }
    
    Shortcut {
        sequence: "Ctrl+D"
        enabled: isConnected
        onActivated: rootStreamClient.disconnect()
    }
}
