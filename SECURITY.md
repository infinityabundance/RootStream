# Security Policy

## Security Architecture (Phase 21)

RootStream implements comprehensive end-to-end encryption and security features:

### **Enhanced Security Features**
- ✅ **Cryptographic Primitives**: AES-256-GCM, ChaCha20-Poly1305 AEAD encryption
- ✅ **Key Exchange**: ECDH (X25519) with X3DH protocol for asynchronous messaging
- ✅ **User Authentication**: Argon2id password hashing with TOTP/2FA support
- ✅ **Session Management**: Secure sessions with perfect forward secrecy
- ✅ **Attack Prevention**: Replay protection, brute force defense, rate limiting
- ✅ **Security Audit**: Comprehensive event logging and audit trails

See [PHASE21_SUMMARY.md](PHASE21_SUMMARY.md) for detailed security documentation.

## Supported versions

RootStream is an early-stage project. Security issues will generally be addressed
on the latest `main` branch and the most recent tagged release.

## Reporting a vulnerability

**Please do not open public GitHub issues for security vulnerabilities.**

Instead:

1. Email the maintainer directly (contact details are listed in the repository).
2. Include:
   - A clear description of the issue
   - Steps to reproduce, if possible
   - Any potential impact you’ve identified

We will:

- Acknowledge your report as soon as reasonably possible.
- Investigate the issue and work on a fix.
- Coordinate disclosure timing with you when appropriate.

## Scope

Security issues of interest include (but are not limited to):

- Remote code execution
- Unauthorized access to streams or encryption keys
- Cryptographic misuse
- Privilege escalation due to RootStream usage

Issues that are **not** considered security vulnerabilities:

- Misconfiguration of the host system (e.g. unsafe firewall rules)
- Compromise of other software on the same host
- Physical access attacks

RootStream aims to use well-vetted cryptographic primitives (via libsodium) and
a minimal attack surface. Security feedback and review are highly appreciated.
