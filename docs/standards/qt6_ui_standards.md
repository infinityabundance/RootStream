# Qt6 UI Standards and Best Practices — RootStream KDE Plasma Client

> **Phase:** PHASE-86.2
> **Applies to:** `clients/kde-plasma-client/src/`, `clients/kde-plasma-client/qml/`
> **Reference:** Qt6 documentation, KDE Human Interface Guidelines 6

---

## 1. Signal/Slot Connections

### Always use new-style (type-safe) syntax

```cpp
// ✅ Correct — type-checked at compile time, refactor-safe
connect(&m_client, &RootStreamClient::connected,
        this, &MainWindow::onConnectionStateChanged);

// ❌ Wrong — string-based, silently breaks on rename
connect(&m_client, SIGNAL(connected()),
        this, SLOT(onConnectionStateChanged()));
```

**Why:** New-style connections fail to compile if the signal or slot is
renamed, preventing silent runtime connection failures.

### Verify connections in tests with QSignalSpy

```cpp
// ✅ Correct
QSignalSpy spy(&client, &RootStreamClient::connected);
client.connectToAddress("host", 1234);
QCOMPARE(spy.count(), 1);

// ❌ Wrong — uses polling, misses race conditions
QTest::qWait(500);
QVERIFY(client.isConnected());
```

---

## 2. Q_PROPERTY Rules

Every property exposed to QML **must** follow this pattern:

```cpp
// ✅ Complete Q_PROPERTY declaration
Q_PROPERTY(QString connectionState
           READ  getConnectionState
           WRITE setConnectionState
           NOTIFY connectionStateChanged)

// The NOTIFY signal must be emitted in the setter:
void MyClass::setConnectionState(const QString &state) {
    if (m_state == state) return;   // avoid spurious notifications
    m_state = state;
    emit connectionStateChanged();  // ← required or QML binding breaks
}
```

**Rules:**
- Never expose raw C++ pointers as Q_PROPERTY (use value types or QObject*
  with parent ownership).
- Always guard against spurious notifications: `if (m_val == val) return;`.
- If a property is read-only from QML, omit WRITE but keep NOTIFY (so QML
  can bind for updates).

---

## 3. QML ↔ C++ Data Flow

### Preferred: Q_PROPERTY bindings (automatic, reactive)

```qml
// QML binds to Q_PROPERTY — updates automatically on NOTIFY signal
Text {
    text: rootStreamClient.connectionState   // always current
}
```

### Use Q_INVOKABLE for actions (not data)

```qml
// User triggers an action
Button {
    onClicked: rootStreamClient.connectToPeer(peerCode.text)
}
```

```cpp
// C++ side
Q_INVOKABLE int connectToPeer(const QString &rootstreamCode);
```

### Avoid signals with complex payloads from C++ to QML

```cpp
// ❌ Wrong — QML cannot easily destructure a struct
emit frameReceived(my_frame_t *frame);

// ✅ Correct — expose individual values as Q_PROPERTY
emit frameReceived(double fps, quint32 latency_ms, const QString &resolution);
```

---

## 4. Threading

- **Never** update UI elements from a non-main thread.
- Use `QMetaObject::invokeMethod(obj, "slotName", Qt::QueuedConnection)` or
  emit a signal across threads — Qt marshals to the main thread automatically
  when using `Qt::QueuedConnection`.
- Network and audio I/O must run on a `QThread` or `QtConcurrent::run()`,
  never on the main thread.

```cpp
// ✅ Correct — thread-safe UI update via queued signal
connect(m_networkThread, &NetworkThread::statsUpdated,
        this,             &MainWindow::updateStats,
        Qt::QueuedConnection);
```

---

## 5. Object Ownership and Memory

- Prefer parent/child ownership: `new Widget(this)` so Qt destructs children
  automatically when the parent is destroyed.
- Avoid `delete` in `~MyClass()` when the object was created with `new X(this)`.
- Use `QScopedPointer<T>` or `std::unique_ptr<T>` for non-QObject owned resources.
- Avoid `QSharedPointer` for QObjects — use parent ownership instead.

---

## 6. QML Component Structure

```
qml/
    Main.qml              — top-level ApplicationWindow
    StreamView.qml        — primary streaming view
    SettingsView.qml      — settings panel
    PeerSelectionView.qml — peer discovery / connection
    StatusBar.qml         — persistent status bar component
    InputOverlay.qml      — on-screen input controls
```

**Rules:**
- Each QML file represents one logical UI component.
- Components must not directly import C++ singletons — use context
  properties or registered QML types instead.
- `id:` names use camelCase: `id: streamView`.
- Anchors are preferred over `x`/`y` positioning (responsive layout).
- Hard-coded pixel sizes are forbidden — use `Units.gridUnit` (KDE) or
  `Qt.application.font.pixelSize` multipliers.

---

## 7. KDE Plasma Specific

- Use `Kirigami.ApplicationWindow` not `QQuickWindow` directly.
- Use `PlasmaCore.Theme.defaultFont` for all text, never hardcode font families.
- Follow KDE HIG for spacing: `Kirigami.Units.largeSpacing` / `smallSpacing`.
- Status messages use `Kirigami.InlineMessage`, not custom `Text` with color.
- Destructive actions (Disconnect, Stop Recording) use `Kirigami.Action`
  with `isDestructive: true`.

---

## 8. Accessibility

- Every interactive element must have `Accessible.name` or `tooltip` set.
- Use `Keys.onReturnPressed` for keyboard activation of buttons.
- Color-only status indicators must have text labels (colorblindness).

```qml
// ✅ Accessible status indicator
Row {
    Rectangle { color: isStreaming ? "green" : "red"; width: 12; height: 12 }
    Text { text: isStreaming ? "Streaming" : "Idle" }
}
```

---

## 9. Test Requirements (Qt6)

Every QObject subclass with signals must have a corresponding `test_*.cpp`:

```cpp
// Required test structure
class TestMyWidget : public QObject {
    Q_OBJECT
private slots:
    void testPropertyHasNotifySignal();   // QMetaObject check
    void testSignalFiresOnChange();       // QSignalSpy check
    void testQMLInvokableExists();        // indexOfMethod check
    void testNullSafety();                // no-crash on empty input
};
QTEST_MAIN(TestMyWidget)
```

Use `QTest::qWait(ms)` only when testing genuinely async operations
(e.g., timers).  For synchronous signal chains, never use `qWait`.

---

## 10. Common Anti-Patterns to Avoid

| Anti-Pattern | Correct Alternative |
|--------------|---------------------|
| `SIGNAL()`/`SLOT()` macros | New-style `&Class::method` connect |
| `QObject::deleteLater()` on stack objects | Use parent ownership |
| `QTimer::singleShot(0, lambda)` to defer UI updates | Emit a queued signal |
| Raw pointers in Q_PROPERTY | `QObject*` with ownership, or value type |
| Calling `qApp->processEvents()` in business logic | Use async signals |
| `Qt::BlockingQueuedConnection` across threads | Use `Qt::QueuedConnection` |
| Storing QML engine pointers in business logic | Expose via context property |
