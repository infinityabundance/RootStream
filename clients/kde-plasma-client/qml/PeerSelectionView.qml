import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        
        Label {
            text: "RootStream KDE Client"
            font.pixelSize: 32
            Layout.alignment: Qt.AlignHCenter
        }
        
        Label {
            text: "No connection"
            font.pixelSize: 18
            color: "gray"
            Layout.alignment: Qt.AlignHCenter
        }
        
        Button {
            text: "Connect to Peer"
            Layout.alignment: Qt.AlignHCenter
            onClicked: connectionDialog.open()
        }
        
        GroupBox {
            title: "Discovered Peers"
            Layout.fillWidth: true
            Layout.preferredWidth: 500
            Layout.preferredHeight: 300
            Layout.alignment: Qt.AlignHCenter
            
            ListView {
                anchors.fill: parent
                model: peerManager
                clip: true
                
                delegate: ItemDelegate {
                    width: parent ? parent.width : 0
                    
                    RowLayout {
                        anchors.fill: parent
                        spacing: 10
                        
                        Column {
                            Layout.fillWidth: true
                            Label {
                                text: model.hostname
                                font.bold: true
                            }
                            Label {
                                text: model.code
                                font.pixelSize: 10
                                color: "gray"
                            }
                        }
                        
                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            color: model.discovered ? "green" : "gray"
                        }
                        
                        Button {
                            text: "Connect"
                            onClicked: rootStreamClient.connectToPeer(model.code)
                        }
                    }
                }
                
                Label {
                    anchors.centerIn: parent
                    text: "No peers discovered yet"
                    visible: peerManager.count === 0
                    color: "gray"
                }
            }
        }
        
        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10
            
            Button {
                text: "Start Discovery"
                onClicked: peerManager.startDiscovery()
            }
            
            Button {
                text: "Add Manual Peer"
                onClicked: addPeerDialog.open()
            }
        }
    }
    
    Dialog {
        id: addPeerDialog
        title: "Add Manual Peer"
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        anchors.centerIn: parent
        
        TextField {
            id: manualPeerField
            placeholderText: "Enter RootStream code"
            width: 300
        }
        
        onAccepted: {
            if (manualPeerField.text.length > 0) {
                peerManager.addManualPeer(manualPeerField.text)
                manualPeerField.text = ""
            }
        }
    }
}
