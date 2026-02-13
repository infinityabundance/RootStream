# RootStream KDE Plasma Client - API Bindings Reference

## Overview

This document describes the C++ API for the RootStream KDE Plasma client. These classes wrap the RootStream C API and expose it to Qt/QML.

---

## RootStreamClient

Main wrapper class for RootStream functionality.

**Header:** `rootstreamclient.h`

### Properties

```cpp
Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
Q_PROPERTY(QString connectionState READ getConnectionState NOTIFY connectionStateChanged)
Q_PROPERTY(QString peerHostname READ getPeerHostname NOTIFY peerHostnameChanged)
```

### Methods

#### Connection Management

```cpp
Q_INVOKABLE int connectToPeer(const QString &rootstreamCode);
```
Connect to a peer using RootStream code (format: `pubkey@hostname`).

**Returns:** 0 on success, -1 on error

**Example:**
```cpp
client.connectToPeer("kXx7YqZ3...@gaming-pc");
```

```cpp
Q_INVOKABLE int connectToAddress(const QString &hostname, quint16 port);
```
Connect to a peer using IP address and port.

```cpp
Q_INVOKABLE void disconnect();
```
Disconnect from current peer.

#### Settings

```cpp
Q_INVOKABLE void setVideoCodec(const QString &codec);
```
Set video codec. Valid values: "h264", "h265", "vp9", "vp8"

```cpp
Q_INVOKABLE void setBitrate(quint32 bitrate_bps);
```
Set streaming bitrate in bits per second (e.g., 10000000 for 10 Mbps).

```cpp
Q_INVOKABLE void setDisplayMode(const QString &mode);
```
Set display mode. Valid values: "windowed", "fullscreen"

```cpp
Q_INVOKABLE void setAudioDevice(const QString &device);
```
Set audio output device.

```cpp
Q_INVOKABLE void setInputMode(const QString &mode);
```
Set input injection mode. Valid values: "uinput", "xdotool"

#### AI Logging

```cpp
Q_INVOKABLE void setAILoggingEnabled(bool enabled);
```
Enable or disable AI logging mode.

```cpp
Q_INVOKABLE QString getLogOutput();
```
Get structured log output.

#### Diagnostics

```cpp
Q_INVOKABLE QString systemDiagnostics();
```
Get system diagnostics information.

**Returns:** Multi-line string with system information

#### State Queries

```cpp
bool isConnected() const;
QString getConnectionState() const;
QString getPeerHostname() const;
```

### Signals

```cpp
void connected();
void disconnected();
void connectionError(const QString &error);
void videoFrameReceived(quint64 timestamp);
void audioSamplesReceived(quint32 sampleCount);
void peerDiscovered(const QString &code, const QString &hostname);
void peerLost(const QString &code);
void statusUpdated(const QString &status);
void performanceMetrics(double fps, quint32 latency_ms, const QString &resolution);
```

---

## PeerManager

Manages peer discovery and list.

**Header:** `peermanager.h`

**Inherits:** `QAbstractListModel`

### Properties

```cpp
Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
```

### Roles

```cpp
enum PeerRoles {
    CodeRole = Qt::UserRole + 1,  // RootStream code
    HostnameRole,                  // Peer hostname
    AddressRole,                   // IP address
    DiscoveredRole                 // Is discovered via mDNS?
};
```

### Methods

```cpp
Q_INVOKABLE void startDiscovery();
Q_INVOKABLE void stopDiscovery();
Q_INVOKABLE void addManualPeer(const QString &code);
Q_INVOKABLE void removePeer(int index);
Q_INVOKABLE void clearPeers();
```

### Signals

```cpp
void countChanged();
void peerAdded(const QString &code);
void peerRemoved(const QString &code);
```

### QML Usage

```qml
ListView {
    model: peerManager
    delegate: ItemDelegate {
        text: model.hostname
        onClicked: rootStreamClient.connectToPeer(model.code)
    }
}
```

---

## SettingsManager

Manages application settings with persistence.

**Header:** `settingsmanager.h`

### Properties

```cpp
Q_PROPERTY(QString codec READ getCodec WRITE setCodec NOTIFY codecChanged)
Q_PROPERTY(int bitrate READ getBitrate WRITE setBitrate NOTIFY bitrateChanged)
```

### Methods

```cpp
Q_INVOKABLE void load();
Q_INVOKABLE void save();
```

### Signals

```cpp
void codecChanged();
void bitrateChanged();
```

### Configuration File

Settings are stored in:
```
~/.config/RootStream/KDE-Client.conf
```

Format:
```ini
[General]
codec=h264
bitrate=10000000
```

---

## LogManager

Manages AI logging integration.

**Header:** `logmanager.h`

### Properties

```cpp
Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
```

### Methods

```cpp
Q_INVOKABLE QString getLogs();
Q_INVOKABLE void clearLogs();
```

### Signals

```cpp
void enabledChanged();
void logAdded(const QString &message);
```

---

## VideoRenderer

(To be implemented)

Handles video frame decoding and OpenGL rendering.

**Header:** `videorenderer.h`

---

## AudioPlayer

(To be implemented)

Handles Opus audio decoding and playback.

**Header:** `audioplayer.h`

---

## InputManager

(To be implemented)

Handles input event capture and injection.

**Header:** `inputmanager.h`

---

## Example Usage

### C++ Example

```cpp
#include "rootstreamclient.h"

RootStreamClient client;

// Connect signals
QObject::connect(&client, &RootStreamClient::connected, []() {
    qInfo() << "Connected to peer";
});

QObject::connect(&client, &RootStreamClient::connectionError, 
                [](const QString &error) {
    qWarning() << "Connection error:" << error;
});

// Configure
client.setVideoCodec("h264");
client.setBitrate(10000000);

// Connect
client.connectToPeer("kXx7YqZ3...@gaming-pc");
```

### QML Example

```qml
import RootStream 1.0

Item {
    RootStreamClient {
        id: client
        
        onConnected: {
            console.log("Connected!")
        }
        
        onConnectionError: (error) => {
            console.error("Error:", error)
        }
    }
    
    Button {
        text: "Connect"
        onClicked: {
            client.setVideoCodec("h264")
            client.setBitrate(10000000)
            client.connectToPeer("kXx7YqZ3...@gaming-pc")
        }
    }
    
    Button {
        text: "Disconnect"
        enabled: client.connected
        onClicked: client.disconnect()
    }
}
```

---

## Thread Safety

- All methods are thread-safe
- Signals are emitted on the main thread
- Network I/O runs on a separate thread internally

## Error Handling

Methods return:
- `0` on success
- `-1` on error

Check signals for detailed error information:
```cpp
connect(&client, &RootStreamClient::connectionError,
        [](const QString &error) {
    // Handle error
});
```

## Memory Management

All objects use Qt's parent-child ownership model:
- Pass `this` or `nullptr` as parent to constructors
- Objects are automatically deleted when parent is deleted

Example:
```cpp
RootStreamClient *client = new RootStreamClient(this);
// Will be deleted when 'this' is deleted
```
