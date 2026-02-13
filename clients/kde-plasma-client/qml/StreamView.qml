import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    
    Rectangle {
        anchors.fill: parent
        color: "black"
        
        // Video stream placeholder
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.8
            height: parent.height * 0.8
            color: "#1a1a1a"
            border.color: "#333"
            border.width: 2
            
            Label {
                anchors.centerIn: parent
                text: "Video Stream\n(Rendering not yet implemented)"
                color: "gray"
                font.pixelSize: 24
                horizontalAlignment: Text.AlignHCenter
            }
        }
        
        // Overlay controls (shown when mouse moves)
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            
            onPositionChanged: {
                overlayControls.visible = true
                overlayHideTimer.restart()
            }
            
            Timer {
                id: overlayHideTimer
                interval: 3000
                onTriggered: {
                    if (!mainWindow.isFullscreen) {
                        overlayControls.visible = false
                    }
                }
            }
            
            Rectangle {
                id: overlayControls
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 60
                color: "#aa000000"
                visible: true
                
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    
                    Label {
                        text: "Connected to: " + rootStreamClient.peerHostname
                        color: "white"
                        Layout.fillWidth: true
                    }
                    
                    Button {
                        text: "Disconnect"
                        onClicked: rootStreamClient.disconnect()
                    }
                }
            }
        }
    }
}
