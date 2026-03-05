# Web Dashboard Audit — RootStream

> **Generated:** 2026-03 · **Phase:** PHASE-85.3
> **Scope:** `frontend/src/` — React 18 + WebSocket + REST API dashboard
> **Pass count:** 5 (as required by the deep-testing prompt)

---

## Executive Summary

The React dashboard is the most functionally complete of the remote clients.
The WebSocket client has real reconnection logic, the API client handles 401
token expiry, and all major dashboard panels render.  However, **7 gaps**
were identified, primarily around missing error boundaries, unvalidated
settings payload, missing test coverage, and lack of production hardening.

---

## Gap Inventory (by file)

### 1. `services/api.js` — APIClient

| Issue | Severity |
|-------|----------|
| `updateVideoSettings()` sends the entire `videoSettings` state object without schema validation.  An empty object or partial update can silently overwrite server settings with `undefined` fields. | 🟠 HIGH |
| No retry logic on network failure (500/503).  User sees a permanent "Error saving" status with no recovery path. | 🟡 MEDIUM |
| `localStorage` used for auth token — vulnerable to XSS.  Production hardening: use `HttpOnly` cookie or in-memory token with refresh. | 🟠 HIGH (security) |
| `verifyAuth()` is defined but never called on app startup — users with expired tokens reach the dashboard and only see errors after the first API call. | 🟡 MEDIUM |

**Recommended subphase:** PHASE-88.5 — Add Zod/yup schema validation to
`updateVideoSettings/Audio/Network`, call `verifyAuth()` in `App.js`
`useEffect`, and migrate token storage to `HttpOnly` cookie.

---

### 2. `services/websocket.js` — WebSocketClient

| Issue | Severity |
|-------|----------|
| `handleMessage()` dispatches by `message.type` but there is no default/unknown-type handler — unknown server messages are silently dropped. | 🟡 MEDIUM |
| `maxReconnectAttempts = 5` — after 5 failures the user sees no feedback and must manually refresh the page. | 🟡 MEDIUM |
| WebSocket URL hardcoded to `ws://localhost:8081` — does not read from environment variable or server-provided configuration.  Production deployments break silently. | 🟠 HIGH |

**Recommended subphase:** PHASE-88.6 — Add `REACT_APP_WS_URL` env variable,
add user-visible reconnection failure toast, and add unknown-message logging.

---

### 3. `components/Dashboard.js`

| Issue | Severity |
|-------|----------|
| No React Error Boundary around the streaming metrics section.  A single WebSocket parse error crashes the entire dashboard tree. | 🟠 HIGH |
| `hostInfo.uptime_seconds` displayed as `Math.floor(.../ 3600)h` only — does not show minutes.  Long-running hosts show "0h" when uptime < 1h. | 🟡 MEDIUM (UX) |
| `wsClient.subscribe('metrics', ...)` is called but `wsClient` is created as a plain object in the component — it is recreated on every render, causing the WebSocket to disconnect/reconnect on every state change. | 🔴 CRITICAL (regression) |

**Recommended subphase:** PHASE-88.7 — Lift `WebSocketClient` instance to
a React context or `useRef`, add `<ErrorBoundary>`, fix uptime formatting.

---

### 4. `components/SettingsPanel.js`

| Issue | Severity |
|-------|----------|
| Settings are loaded in `useEffect([])` but there is no loading state — the panel renders empty for a flash before data arrives (Layout Shift). | 🟡 MEDIUM (UX) |
| Saving one settings category (e.g., video) does not prevent concurrent saves of another category, potentially sending two conflicting requests. | 🟡 MEDIUM |
| No "Reset to defaults" button — users cannot recover from bad settings without direct API access. | 🟡 MEDIUM (UX) |

---

### 5. `components/PerformanceGraphs.js`

| Issue | Severity |
|-------|----------|
| Recharts data arrays grow unboundedly as WebSocket metrics arrive — no max-window trimming.  Memory usage grows until the tab is refreshed. | 🟠 HIGH (memory leak) |
| Y-axis scales are fixed at compile time — do not adapt to actual data range.  High-bitrate streams clip the chart. | 🟡 MEDIUM (UX) |

**Recommended subphase:** PHASE-88.8 — Cap metrics arrays at 300 entries
(rolling window), use Recharts `domain={['auto', 'auto']}`.

---

## Missing Test Coverage

The project has `@testing-library/react` and `jest` in devDependencies but
**zero test files exist** under `frontend/src/`.

**Minimum recommended tests:**
- `Dashboard.test.js` — renders without crash, shows "Idle" when not streaming
- `SettingsPanel.test.js` — save button calls `APIClient.updateVideoSettings`
- `WebSocketClient.test.js` — reconnection logic exercised with fake timers
- `APIClient.test.js` — 401 response clears token and redirects

**Recommended subphase:** PHASE-89.1 — Create all 4 test files using
`@testing-library/react` and `msw` for API mocking.

---

## React / UI Best Practices Gap Summary

| Best Practice | Status |
|---------------|--------|
| Error Boundaries around async data | ❌ Missing |
| Loading / skeleton states | ❌ Missing |
| Accessible labels on form inputs (`aria-label`) | ⚠️ Partial |
| Responsive layout (mobile dashboard view) | ⚠️ Partial (CSS exists, tested on 1280px only) |
| Dark/light theme toggle | ❌ Missing |
| Keyboard navigation for settings panel | ❌ Missing |

---

## Priority Order for Implementation

1. WebSocket instance lifecycle fix (PHASE-88.7) — critical regression
2. Memory leak in `PerformanceGraphs` (PHASE-88.8)
3. Token storage security (PHASE-88.5)
4. Test coverage (PHASE-89.1)
5. `verifyAuth()` on startup (PHASE-88.5)
6. Settings validation (PHASE-88.5)
