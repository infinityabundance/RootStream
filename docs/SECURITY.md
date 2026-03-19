# Security Policy

## Security Architecture

RootStream uses audited cryptographic primitives via libsodium for all
encryption and authentication operations. The cryptographic design is:

- **Ed25519** keypairs for device identity (one keypair per machine, generated once)
- **X25519 ECDH** for session key derivation from device keypairs
- **ChaCha20-Poly1305** authenticated encryption for all stream packets
- **Monotonic nonces** for replay attack prevention

> **Important**: RootStream uses audited algorithms (Ed25519, ChaCha20-Poly1305
> via libsodium), but the RootStream implementation itself has **not** undergone
> independent security audit. Use on trusted networks only.

For a full threat model including trust boundaries, threat scenarios, and
residual risks, see [`docs/THREAT_MODEL.md`](THREAT_MODEL.md).

### What is currently NOT implemented

The following are **not** part of the current supported product:
- Argon2id password hashing (account system not present in supported path)
- TOTP/2FA (no account system)
- Rate limiting at the network layer
- At-rest encryption of recording files

## Supported Versions

RootStream is an early-stage project. Security issues will generally be
addressed on the latest `main` branch and the most recent tagged release.

## Reporting a Vulnerability

**Please do not open public GitHub issues for security vulnerabilities.**

Instead:

1. Email the maintainer directly (contact details are listed in the repository).
2. Include:
   - A clear description of the issue
   - Steps to reproduce, if possible
   - Any potential impact you've identified

We will:

- Acknowledge your report as soon as reasonably possible.
- Investigate the issue and work on a fix.
- Coordinate disclosure timing with you when appropriate.

## Scope

Security issues of interest include (but are not limited to):

- Remote code execution
- Unauthorized access to streams or encryption keys
- Cryptographic misuse or implementation errors
- Privilege escalation due to RootStream usage
- Private key exposure

Issues that are **not** considered security vulnerabilities:

- Misconfiguration of the host system (e.g. unsafe firewall rules)
- Compromise of other software on the same host
- Physical access attacks
- Streaming over untrusted public networks without VPN

RootStream aims to use well-vetted cryptographic primitives (via libsodium) and
a minimal attack surface. Security feedback and review are highly appreciated.
