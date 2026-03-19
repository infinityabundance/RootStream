# RootStream Release Process

This document defines the release process, versioning policy, ship/no-ship
criteria, and known-issues management for the supported RootStream product.

---

## Versioning Policy

RootStream uses Semantic Versioning (`MAJOR.MINOR.PATCH`):

| Component | When it increments |
|---|---|
| `MAJOR` | Breaking change to wire protocol, key format, or CLI interface |
| `MINOR` | New supported feature on the canonical path; new platform support |
| `PATCH` | Bug fix, documentation improvement, dependency update |

The current version is defined in:
- `CMakeLists.txt`: `project(rootstream VERSION ...)`
- `include/rootstream.h`: `#define ROOTSTREAM_VERSION "..."`
- `Makefile`: `VERSION ?= ...` (if defined)

Both must be updated together for a release.

### Branch Model

| Branch | Purpose |
|---|---|
| `main` | Stable, releasable. Every commit here should pass CI. |
| `develop` | Integration branch for ongoing work |
| `feature/*` | Individual feature branches; merged via PR |
| `release/vX.Y.Z` | Release preparation (version bump, release notes, tag) |

---

## Release Checklist

Before tagging a release:

### Pre-release Validation

- [ ] All CI jobs pass on `main` (build, unit-tests, integration-tests, format-check, sanitizer)
- [ ] `./rootstream --version` outputs the correct version string
- [ ] `./rootstream --help` output is accurate and reflects current commands
- [ ] `docs/CORE_PATH.md` validation steps pass on a clean machine
- [ ] `scripts/demo.sh` exits with code 0
- [ ] `make test-build && ./tests/unit/test_crypto && ./tests/unit/test_encoding` pass

### Documentation

- [ ] `README.md` reflects current supported product definition
- [ ] `docs/SUPPORT_MATRIX.md` maturity labels are up to date
- [ ] `docs/ROADMAP.md` future items are accurate (no current-release items left as future)
- [ ] `CHANGELOG.md` entry added for the release
- [ ] `docs/KNOWN_ISSUES.md` is up to date

### Version Bump

- [ ] `CMakeLists.txt` version updated
- [ ] `include/rootstream.h` `ROOTSTREAM_VERSION` updated
- [ ] Git tag created: `git tag -a vX.Y.Z -m "Release vX.Y.Z"`

### Artifacts

- [ ] Linux binary built and verified: `file rootstream && ldd rootstream`
- [ ] PKGBUILD updated if packaging for Arch Linux
- [ ] GitHub Release created with changelog excerpt and binary

---

## Ship / No-Ship Criteria

### SHIP when:

- All unit tests pass (crypto, encoding)
- Integration test script passes (`tests/integration/test_stream.sh`)
- The canonical demo (`scripts/demo.sh`) exits successfully
- No known regressions from the previous release
- Security-relevant changes have been reviewed

### NO-SHIP when:

- Any CI job is red on `main`
- A regression in the supported path (host startup, QR, identity generation) is known
- A security-relevant issue has been discovered and is unpatched
- The version string in code and tag do not match

---

## Known-Issues Management

### Severity Levels

| Level | Description | Example |
|---|---|---|
| P0 Blocker | Prevents the supported path from working | Host crashes on startup |
| P1 High | Significantly degrades supported path | Audio desync on first connection |
| P2 Medium | Workaround exists; not blocking | mDNS doesn't work without Avahi |
| P3 Low | Minor inconvenience | Minor doc typo |

### Tracking

Known issues are tracked in [`docs/KNOWN_ISSUES.md`](KNOWN_ISSUES.md).
New issues discovered during release validation are added before tagging.

---

## Post-Release Steps

1. Push the release tag to GitHub: `git push origin vX.Y.Z`
2. Create a GitHub Release using the tag
3. Announce in relevant channels (if applicable)
4. Update `develop` branch version to next `PATCH+1-dev` pre-release marker

---

## Hotfix Process

For urgent P0 fixes:

1. Branch from the release tag: `git checkout -b hotfix/vX.Y.Z+1 vX.Y.Z`
2. Apply the minimal fix
3. Run the pre-release validation checklist
4. Tag as `vX.Y.(Z+1)`
5. Merge back to `main` and `develop`
