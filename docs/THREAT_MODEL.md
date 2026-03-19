# RootStream Threat Model

This document describes the trust boundaries, threat scenarios, and security
assumptions for the supported RootStream product path (Linux host → Linux peer
on a private LAN).

This is a first-pass threat model. The implementation has not undergone
independent security audit. Feedback and review are welcomed.

See also: [`docs/SECURITY.md`](SECURITY.md)

---

## Product Scope for This Model

| In scope | Out of scope (current) |
|---|---|
| Linux host streaming to Linux peer | Cloud-managed deployment |
| Direct P2P on LAN | Internet streaming without VPN |
| Ed25519/ChaCha20 encrypted streams | WebRTC or browser clients |
| CLI and local config | Account-based systems |

---

## Trust Boundaries

```
┌─────────────────────────────────────────────────────────────────────┐
│  HOST MACHINE (trusted)                                              │
│                                                                      │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────────────┐  │
│  │  Display /   │    │  rootstream  │    │  ~/.config/rootstream │  │
│  │  GPU / DRM   │───▶│  host        │───▶│  identity.key (priv) │  │
│  └──────────────┘    └──────┬───────┘    └──────────────────────┘  │
│                             │                                        │
└─────────────────────────────┼────────────────────────────────────────┘
                              │  UDP (encrypted)
                   ╔══════════▼═══════════════╗
                   ║  NETWORK BOUNDARY         ║  ← Trust boundary
                   ║  LAN / controlled path    ║
                   ╚══════════╤═══════════════╝
                              │
┌─────────────────────────────┼────────────────────────────────────────┐
│  PEER MACHINE (trusted, must present correct key)                    │
│                                                                      │
│                      ┌──────▼───────┐                               │
│                      │  rootstream  │                               │
│                      │  connect     │                               │
│                      └──────────────┘                               │
└─────────────────────────────────────────────────────────────────────┘
```

The network boundary is the primary trust boundary. Traffic crossing it
is encrypted and authenticated. Traffic within a host machine is assumed
trusted (no intra-process isolation).

---

## Assets

| Asset | Sensitivity | Where stored |
|---|---|---|
| Ed25519 private key | **Critical** — identity theft if leaked | `~/.config/rootstream/identity.key` (file permissions: 600) |
| Ed25519 public key | Low — shared intentionally via peer code | `~/.config/rootstream/identity.pub` |
| Session shared secret | High — derived per-session, ephemeral | In-process memory only |
| Stream content | Variable — user's display pixels and audio | In-transit (encrypted); not persisted unless recording enabled |
| Peer code | Medium — reveals host identity/hostname | Shared deliberately; do not post publicly |
| Config file | Low | `~/.config/rootstream/config.ini` |
| Recording files | Variable | User-specified path (if `--record` enabled) |

---

## Threat Scenarios

### T-1: Network eavesdropping

**Threat**: An attacker captures UDP traffic between host and peer.

**Mitigation**: All stream packets are encrypted with ChaCha20-Poly1305
(via libsodium). The session key is derived from X25519 ECDH using each
device's Ed25519 keypair. An eavesdropper cannot decrypt packets without
the private keys.

**Residual risk**: None for passive interception.  
**Unproven**: Independent security audit of the libsodium usage in
`src/crypto.c` has not been performed.

---

### T-2: Peer impersonation / MITM

**Threat**: An attacker intercepts the connection and pretends to be either
the host or the peer.

**Mitigation**: The peer code (`base64(pubkey)@hostname`) is exchanged
out-of-band (QR code or text). On connection, both sides authenticate using
their Ed25519 private keys. A MITM attacker without the private key cannot
authenticate.

**Residual risk**: If the peer code is exchanged over an untrusted channel
(e.g., posted publicly), an attacker who controls a hostname could substitute
their public key. **Users should only share peer codes with trusted peers.**

**Unproven**: Formal verification of the handshake protocol
(`src/session_hs/`) has not been performed.

---

### T-3: Replay attacks

**Threat**: An attacker captures and replays encrypted packets.

**Mitigation**: Each packet includes a monotonically increasing nonce.
Packets with a nonce lower than the last accepted nonce are rejected.
libsodium's `crypto_box_open_easy` / `crypto_secretbox_open_easy` includes
an authentication tag that prevents tag forgery.

**Residual risk**: Nonce state is not persisted across restarts; replay
across sessions is not prevented (same key pair, new session starts at nonce 0).
This is acceptable for the current LAN use case.

---

### T-4: Private key theft

**Threat**: An attacker gains read access to `~/.config/rootstream/identity.key`.

**Mitigation**: The key file is written with `chmod 600` on creation.
File permissions prevent unprivileged processes from reading it.

**Residual risk**: Root-level access on the host machine bypasses file
permissions. This is outside the threat model scope (physical access /
privilege escalation).

**Recommendation**: Back up the key file and rotate it if you suspect
compromise (delete the key file; a new keypair will be generated on next
run).

---

### T-5: Denial of service (DoS)

**Threat**: An attacker floods the host UDP port to disrupt streaming.

**Mitigation**: Packets failing authentication are dropped before
processing. The host does not allocate expensive resources before
authentication.

**Residual risk**: High-rate UDP flood can saturate network bandwidth
or cause packet loss. RootStream does not implement rate limiting at the
network layer. Firewalling the streaming port to known peer IPs is
recommended on public networks.

---

### T-6: uinput privilege escalation (host side)

**Threat**: The host process uses `/dev/uinput` to inject input events.
If compromised, it could inject arbitrary input to the host machine.

**Mitigation**: uinput access is limited to users in the `input` group.
The RootStream host only injects events received from authenticated peers.

**Residual risk**: If a peer is compromised, they can inject arbitrary
input to the host display. Only connect to peers you trust.

---

### T-7: Recording file exposure

**Threat**: The `--record` flag saves stream content to disk.

**Mitigation**: Recording is opt-in and explicit. The file path is
user-specified. Files are not encrypted at rest.

**Recommendation**: Store recordings in an encrypted volume if stream
content is sensitive.

---

## Security Controls Summary

| Control | Implemented | Notes |
|---|---|---|
| Ed25519 keypair authentication | ✅ | Via libsodium `crypto_sign_*` |
| ChaCha20-Poly1305 stream encryption | ✅ | Via libsodium `crypto_secretbox_*` |
| X25519 ECDH session key derivation | ✅ | Via libsodium `crypto_box_*` |
| Monotonic nonce / replay prevention | ✅ | Per-packet nonce counter |
| Private key file permissions (600) | ✅ | On Linux; Windows path TBD |
| Peer code out-of-band exchange | ✅ | QR code or text copy |
| Rate limiting | ❌ | Not implemented |
| IP allowlist / firewall | ❌ | Recommended but not enforced |
| At-rest recording encryption | ❌ | Not implemented |
| Independent security audit | ❌ | Planned for future phase |

---

## What RootStream Does Not Protect Against

- Compromised host operating system or kernel
- Physical access to either machine
- Malicious peer (input injection from trusted peer is by design)
- Streaming over untrusted networks (use VPN)
- Long-term key management or key rotation workflows

---

## Reporting Security Issues

**Do not open public GitHub issues for security vulnerabilities.**

See [`docs/SECURITY.md`](SECURITY.md) for the vulnerability reporting process.
