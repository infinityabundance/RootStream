# RootStream Documentation Style Guide

This guide defines style rules for RootStream documentation. Apply it when
writing or reviewing docs in `docs/`, `README.md`, and `CONTRIBUTING.md`.

---

## Principles

1. **Prove, don't posture.** Every feature claim in a user-facing document
   must be backed by a CI check, integration test, or explicit caveat.
2. **Be honest about maturity.** Use the maturity vocabulary from
   [`docs/SUPPORT_MATRIX.md`](SUPPORT_MATRIX.md): Supported / Preview /
   Experimental / Roadmap. Never use `✅` for unimplemented features.
3. **Link to truth sources.** When stating a fact that has an authoritative
   document, link to that document. Avoid duplication.
4. **Use active voice.** Prefer "Run `rootstream host`" over "The host can be
   started by running `rootstream host`."
5. **Short sentences.** Target 15–20 words per sentence. Complex technical
   content is fine; unnecessary verbosity is not.
6. **No marketing language.** Avoid "world-class", "blazing-fast", "industry-leading"
   without measurement evidence. Use measured numbers instead.

---

## Structure

### Top-level docs (files directly in `docs/`)

- Start with a one-line summary of what the document covers.
- Follow with "See also" cross-links for related documents.
- Use `##` for top-level sections, `###` for subsections.
- Include a `---` horizontal rule between major sections.

### Tables

Use Markdown tables for:
- Feature matrices
- Dependency lists (name, pkg-config name, effect when absent)
- Command references (command, arguments, output)
- Status tracking (ID, task, status)

Table alignment: left-align all columns in source; rendered alignment is fine.

### Code blocks

Always specify the language for syntax highlighting:

```bash   # Shell commands
```c      # C code
```cmake  # CMake
```yaml   # YAML / GitHub Actions
```

Use `$` prefix only for commands a user runs, not for output lines.

### Status icons

| Icon | Meaning |
|---|---|
| ✅ | Verified and working |
| 🟢 | Complete / supported |
| 🟡 | Partial / in progress |
| 🔴 | Not started / not supported |
| ⚪ | Future / roadmap |
| ⚠️ | Warning / caveat |
| ❌ | Failure / not recommended |

---

## Vocabulary

Use these canonical terms consistently across all docs:

| Preferred | Avoid |
|---|---|
| host | server, broadcaster, streamer |
| peer / client | viewer, receiver |
| peer code | RootStream code, connection code (inconsistent) |
| LAN / local network | WiFi, Ethernet (unless specifically relevant) |
| `rootstream host` | "run in host mode" |
| `rootstream connect` | "run as client" |
| Ed25519 keypair | keys, certificates, credentials |
| session | connection (unless referring to TCP) |
| build flag | compile flag, make variable (use in context) |

See [`docs/GLOSSARY.md`](GLOSSARY.md) for the full vocabulary reference.

---

## Maturity Labels

From `docs/SUPPORT_MATRIX.md`:

| Label | Definition |
|---|---|
| **Supported** | Works end-to-end on the canonical path; CI-validated |
| **Preview** | Functionally implemented; not CI-validated for all paths |
| **Experimental** | Code present; not stable; use at your own risk |
| **Roadmap** | Planned; no code yet |

**Never use ✅ for Experimental or Roadmap items** in user-facing documents.

---

## File Naming

| Type | Convention | Example |
|---|---|---|
| Docs in `docs/` | `SCREAMING_SNAKE.md` | `SUPPORT_MATRIX.md` |
| Sub-docs in `docs/<dir>/` | `SCREAMING_SNAKE.md` | `BOUNDARY_RULES.md` |
| Scripts in `scripts/` | `kebab-case.sh` | `setup-dev.sh` |
| C sources | `snake_case.c` | `audio_playback_alsa.c` |
| C headers | `snake_case.h` | `rootstream.h` |

---

## Cross-Linking Rules

1. Link from README.md to `docs/` using relative paths.
2. Link between `docs/` files using `[anchor text](FILENAME.md)`.
3. Do not link to external URLs from core docs unless required for
   dependency attribution.
4. Every new document must be linked from at least one existing document.

---

## Checklist for New Documents

Before adding a new doc:

- [ ] Does a single existing document cover this topic? If so, extend it instead.
- [ ] Is the maturity level of described features clearly stated?
- [ ] Are all commands verified to work (or caveated as needing hardware)?
- [ ] Is the new doc linked from at least one existing doc?
- [ ] Does the doc follow the Structure rules above?
