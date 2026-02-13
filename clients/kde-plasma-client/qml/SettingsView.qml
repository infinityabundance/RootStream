import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: root
    title: "Settings"
    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Apply
    modal: true
    anchors.centerIn: parent
    width: 500
    height: 400
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 15
        
        GroupBox {
            title: "Video"
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                Label {
                    text: "Codec:"
                }
                
                ComboBox {
                    id: codecComboBox
                    Layout.fillWidth: true
                    model: ["h264", "h265", "vp9", "vp8"]
                    currentIndex: model.indexOf(settingsManager.codec)
                }
                
                Label {
                    text: "Bitrate: " + (bitrateSlider.value / 1000000).toFixed(1) + " Mbps"
                }
                
                Slider {
                    id: bitrateSlider
                    Layout.fillWidth: true
                    from: 1000000
                    to: 50000000
                    value: settingsManager.bitrate
                    stepSize: 1000000
                }
            }
        }
        
        GroupBox {
            title: "Audio"
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                CheckBox {
                    id: audioEnabledCheckBox
                    text: "Enable audio"
                    checked: true
                }
                
                Label {
                    text: "Audio device:"
                }
                
                ComboBox {
                    id: audioDeviceComboBox
                    Layout.fillWidth: true
                    model: ["Default", "PulseAudio", "PipeWire", "ALSA"]
                }
            }
        }
        
        GroupBox {
            title: "Input"
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                Label {
                    text: "Input mode:"
                }
                
                ComboBox {
                    id: inputModeComboBox
                    Layout.fillWidth: true
                    model: ["uinput", "xdotool"]
                }
            }
        }
        
        GroupBox {
            title: "Advanced"
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                CheckBox {
                    id: aiLoggingCheckBox
                    text: "Enable AI logging (debug mode)"
                    checked: logManager.enabled
                }
            }
        }
        
        Item {
            Layout.fillHeight: true
        }
    }
    
    onAccepted: {
        applySettings()
    }
    
    onApplied: {
        applySettings()
    }
    
    function applySettings() {
        settingsManager.codec = codecComboBox.currentText
        settingsManager.bitrate = bitrateSlider.value
        settingsManager.save()
        
        rootStreamClient.setVideoCodec(codecComboBox.currentText)
        rootStreamClient.setBitrate(bitrateSlider.value)
        rootStreamClient.setAudioDevice(audioDeviceComboBox.currentText)
        rootStreamClient.setInputMode(inputModeComboBox.currentText)
        logManager.enabled = aiLoggingCheckBox.checked
    }
}
