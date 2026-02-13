# RootStream KDE Plasma Client - Screenshots & UI Overview

## Main Window (Disconnected State)

When not connected, the client shows the peer selection view:

```
┌─────────────────────────────────────────────────────────────┐
│ File  View  Help                                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│           RootStream KDE Client                             │
│                                                             │
│               No connection                                 │
│                                                             │
│           [  Connect to Peer  ]                             │
│                                                             │
│     ┌───────────  Discovered Peers  ───────────┐           │
│     │                                           │           │
│     │  ○ gaming-pc      (discovered) [Connect] │           │
│     │  ○ workstation    (manual)     [Connect] │           │
│     │                                           │           │
│     └───────────────────────────────────────────┘           │
│                                                             │
│     [ Start Discovery ]  [ Add Manual Peer ]               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Connection Dialog

```
┌──────────── Connect to Peer ────────────┐
│                                          │
│  Enter RootStream Code or IP:Port       │
│  ┌────────────────────────────────────┐ │
│  │ kXx7YqZ3...@hostname               │ │
│  └────────────────────────────────────┘ │
│                                          │
│  Or select from discovered peers:        │
│  ┌────────────────────────────────────┐ │
│  │ gaming-pc (discovered)             │ │
│  │ workstation (manual)               │ │
│  └────────────────────────────────────┘ │
│                                          │
│              [ OK ]  [ Cancel ]          │
└──────────────────────────────────────────┘
```

## Main Window (Connected State - Windowed)

When connected, the client shows the stream view:

```
┌─────────────────────────────────────────────────────────────┐
│ File  View  Help                                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│                                                             │
│               ┌─────────────────────┐                       │
│               │                     │                       │
│               │   Video Stream      │                       │
│               │  (Rendering not     │                       │
│               │  yet implemented)   │                       │
│               │                     │                       │
│               └─────────────────────┘                       │
│                                                             │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│ ● Connected  │ FPS: --  │ Latency: -- ms  │ Peer: gaming-pc │
└─────────────────────────────────────────────────────────────┘
```

## Fullscreen Mode (F11)

```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│                                                             │
│                                                             │
│                                                             │
│                      Video Stream                           │
│                  (Full screen display)                      │
│                                                             │
│                                                             │
│                                                             │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│ Connected to: gaming-pc                    [  Disconnect  ] │ (Auto-hide overlay)
└─────────────────────────────────────────────────────────────┘
```

## Settings Dialog

```
┌──────────────────── Settings ──────────────────────┐
│                                                    │
│  ┌── Video ───────────────────────────────────┐   │
│  │                                             │   │
│  │  Codec:  [ h264 ▼ ]                        │   │
│  │          h264 / h265 / vp9 / vp8           │   │
│  │                                             │   │
│  │  Bitrate: 10.0 Mbps                        │   │
│  │  ├─────────●─────────────────────┤         │   │
│  │  1 Mbps                      50 Mbps       │   │
│  └─────────────────────────────────────────────┘   │
│                                                    │
│  ┌── Audio ───────────────────────────────────┐   │
│  │                                             │   │
│  │  ☑ Enable audio                            │   │
│  │                                             │   │
│  │  Audio device: [ Default ▼ ]               │   │
│  │                Default / PulseAudio / ...  │   │
│  └─────────────────────────────────────────────┘   │
│                                                    │
│  ┌── Input ───────────────────────────────────┐   │
│  │                                             │   │
│  │  Input mode: [ uinput ▼ ]                  │   │
│  │              uinput / xdotool              │   │
│  └─────────────────────────────────────────────┘   │
│                                                    │
│  ┌── Advanced ────────────────────────────────┐   │
│  │                                             │   │
│  │  ☐ Enable AI logging (debug mode)          │   │
│  └─────────────────────────────────────────────┘   │
│                                                    │
│      [ OK ]  [ Cancel ]  [ Apply ]                │
└────────────────────────────────────────────────────┘
```

## Diagnostics Dialog

```
┌────────────── System Diagnostics ──────────────┐
│                                                │
│  ┌──────────────────────────────────────────┐ │
│  │ RootStream KDE Client                    │ │
│  │ Version: 1.0.0                           │ │
│  │ Connected: Yes                           │ │
│  │ Connection State: Connected              │ │
│  │ Peer: gaming-pc                          │ │
│  │ Codec: H.264                             │ │
│  │ Bitrate: 10.0 Mbps                       │ │
│  │                                          │ │
│  │ Hardware Acceleration: VA-API Available  │ │
│  │ Audio Backend: PulseAudio                │ │
│  │ Network: UDP Port 9876                   │ │
│  │                                          │ │
│  │ [... additional diagnostics ...]         │ │
│  └──────────────────────────────────────────┘ │
│                                                │
│                      [ Close ]                  │
└────────────────────────────────────────────────┘
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| **F11** | Toggle fullscreen |
| **Escape** | Exit fullscreen |
| **Ctrl+Q** | Quit application |
| **Ctrl+D** | Disconnect from peer |

## Menu Structure

```
File
├─ Connect...
├─ Disconnect
├─ ──────────
├─ Settings
├─ ──────────
└─ Quit

View
├─ Fullscreen
└─ Toggle Status Bar

Help
├─ About
└─ Diagnostics
```

## Color Scheme

The UI follows KDE Plasma theme colors:
- **Primary**: #4a90e2 (Blue accent)
- **Background**: System theme
- **Text**: System theme
- **Connected indicator**: Green dot
- **Disconnected indicator**: Red dot
- **Overlay**: Semi-transparent black (#aa000000)

## Responsive Design

The UI adapts to different window sizes:
- **Minimum**: 800x600
- **Default**: 1280x720
- **Fullscreen**: Native resolution

## Accessibility

- All controls accessible via keyboard
- Tab navigation support
- Screen reader compatible (Qt accessibility)
- High contrast mode support

## Notes

- Screenshots are ASCII art representations
- Actual UI uses Qt Quick Controls 2 with native theming
- Visual design follows KDE Human Interface Guidelines
- Animations and transitions not shown in ASCII art
