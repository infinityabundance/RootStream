import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    height: 40
    color: "#aa000000"
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 20
        
        Label {
            text: "●"
            color: rootStreamClient.connected ? "green" : "red"
            font.pixelSize: 16
        }
        
        Label {
            text: rootStreamClient.connectionState
            color: "white"
        }
        
        Rectangle {
            width: 1
            Layout.fillHeight: true
            color: "#555"
        }
        
        Label {
            text: "FPS: --"
            color: "white"
        }
        
        Label {
            text: "Latency: -- ms"
            color: "white"
        }
        
        Label {
            text: "Resolution: --"
            color: "white"
        }
        
        Item {
            Layout.fillWidth: true
        }
        
        Label {
            text: rootStreamClient.connected ? ("Peer: " + rootStreamClient.peerHostname) : "Not connected"
            color: "white"
        }
    }
}
