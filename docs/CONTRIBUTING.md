# Contributing to RootStream 🚀

First off, thank you for considering contributing to RootStream!  
This project was born from frustration with broken streaming solutions on Linux, and every contribution helps make it better.

╔════════════════════════════════════════════════╗  
║  All Contributions Welcome!                    ║  
║  • Bug reports                                 ║  
║  • Feature requests                            ║  
║  • Documentation improvements                  ║  
║  • Code contributions                          ║  
║  • Testing on different hardware               ║  
╚════════════════════════════════════════════════╝  

---

## Table of Contents

- [Project Philosophy](#project-philosophy)
- [Code of Conduct](#code-of-conduct)
- [Ways to Contribute](#ways-to-contribute)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Code Guidelines](#code-guidelines)
- [Code Quality Checklist](#code-quality-checklist)
- [Commit Messages](#commit-messages)
- [Pull Request Process](#pull-request-process)
- [Testing](#testing)
- [Priority Areas](#priority-areas)
- [Communication](#communication)
- [Recognition](#recognition)
- [Getting Help](#getting-help)
- [Security](#security)
- [Licensing](#licensing)
- [Final Notes](#final-notes)

---

## Project Philosophy

RootStream is a **Linux-native, low-latency, peer-to-peer game streaming system**.

Before contributing, please understand the core principles:

- **Linux-first**  
  RootStream targets Linux hosts and clients. Cross-platform support is a long-term goal, not a current requirement.

- **Low latency over convenience**  
  Direct DRM/KMS capture, hardware encoders, and minimal abstraction layers are intentional.  
  Features that increase latency or complexity without clear benefit will likely be rejected.

- **Minimal dependencies**  
  Dependencies should be widely available on Linux, justified, and well-maintained.

- **No custom cryptography**  
  RootStream uses well-established primitives via libsodium. PRs introducing custom crypto
  or protocol changes without strong justification will not be accepted.

- **Clarity beats cleverness**  
  Readable, auditable code is preferred over “clever” implementations.

- **User freedom, security, simplicity, performance, quality**  
  - No accounts, no tracking, no lock-in  
  - End-to-end encryption with audited crypto  
  - Do one thing well, avoid scope creep  
  - Low latency, low resource usage  
  - Excellent error messages and good documentation  

When in doubt, ask: **“Does this align with these principles?”**

---

## Code of Conduct

### Our Pledge

We're here to build great software and help each other.  
Be kind, be respectful, be constructive.

### Expected Behavior

- **Be welcoming** – Everyone was a beginner once.  
- **Be respectful** – Disagree with ideas, not people.  
- **Be constructive** – Criticism should include suggestions.  
- **Be patient** – We're all volunteers with day jobs.  
- **Give credit** – Acknowledge others' contributions.

### Unacceptable Behavior

- Harassment, discrimination, or personal attacks  
- Trolling, insulting comments, or unconstructive criticism  
- Publishing others' private information  
- Any conduct inappropriate in a professional setting  

**Violations?** Contact the maintainers.  
We'll handle it privately and professionally.

---

## Ways to Contribute

You don’t need to write code to help!

### 🐛 Bug Reports

Found a bug? Excellent! Please open an issue with:

**Description**

- Clear description of the bug.

**Steps to Reproduce**

```text
1. Start RootStream with ...
2. Click on ...
3. See error
```

**Expected Behavior**

- What should happen.

**Actual Behavior**

- What actually happens.

**System Information**

- OS: (e.g. Arch Linux / Ubuntu 24.04 / Fedora 41)  
- Kernel: (e.g. 6.7.1)  
- GPU: (e.g. Intel UHD 730 / AMD RX 6800 / NVIDIA RTX 4070)  
- RootStream version: (e.g. 1.0.0 or commit hash)

**Logs**

```text
[paste relevant logs here]
```

**Screenshots**

- If applicable, include screenshots or short clips.

**Pro tip:** Run with verbose logging:

```bash
rootstream --service --verbose 2>&1 | tee rootstream.log
```

---

### 💡 Feature Requests

Have an idea? Open an issue with:

- **Use case** – What problem does this solve?  
- **Proposed solution** – How would it work?  
- **Alternatives considered** – What else did you think about?  
- **Willingness to implement** – Can you code it yourself?

Please also mention how it affects:

- Latency  
- Complexity  
- Dependencies

---

### 📚 Documentation

Documentation is just as important as code! Ways to help:

- Fix typos or clarify confusing sections  
- Add more examples  
- Translate documentation (see i18n roadmap)  
- Write tutorials or guides  
- Improve code comments  

---

### 🧪 Testing

Don’t code? You can still help!

- Test on different hardware (especially AMD/NVIDIA GPUs)  
- Try edge cases and report results  
- Verify fixes in pull requests  
- Test on different distros (Ubuntu, Fedora, Arch, etc.)

---

### 🎨 Design

- UI/UX improvements for tray app  
- Icon design (we need better icons!)  
- QR code styling  
- Marketing materials  

---

## Getting Started

### Prerequisites (Arch Linux example)

```bash
# Arch Linux dependencies
sudo pacman -S base-devel git libdrm libva gtk3 libsodium qrencode libpng

# Development tools
sudo pacman -S gdb valgrind clang-tools-extra cppcheck

# Documentation tools (optional)
sudo pacman -S doxygen graphviz
```

Equivalent packages on other distros are welcome as PRs to the docs.

### Fork and Clone

```bash
# Fork on GitHub, then:
git clone https://github.com/YOUR_USERNAME/RootStream.git
cd RootStream

# Add upstream remote
git remote add upstream https://github.com/infinityabundance/RootStream.git
```

### Build and Test

```bash
# Build with debug symbols
make DEBUG=1

# Run tests (when available)
make test

# Check for static analysis issues
make check
```

### Create a Branch

```bash
# Update your fork
git checkout main
git pull upstream main

# Create feature branch
git checkout -b feature/awesome-feature

# Or fix branch
git checkout -b fix/bug-description
```

---

## Development Setup

### Recommended Tools

**Editor/IDE**

- VSCode with C/C++ extension  
- CLion (JetBrains)  
- Neovim with LSP (clangd)  
- Any editor with clangd support  

**Debugger**

```bash
# Run with GDB
gdb ./rootstream
(gdb) run --service

# Or with valgrind for memory leaks
valgrind --leak-check=full ./rootstream --service
```

**Static Analysis**

```bash
# Check code quality
make check

# Format code (uses clang-format)
make format
```

### VSCode Configuration (Optional)

Create `.vscode/c_cpp_properties.json`:

```json
{
  "configurations": [
    {
      "name": "Linux",
      "includePath": [
        "${workspaceFolder}/**",
        "/usr/include/libdrm",
        "/usr/include/gtk-3.0",
        "/usr/include/glib-2.0",
        "/usr/lib/glib-2.0/include"
      ],
      "defines": [],
      "compilerPath": "/usr/bin/gcc",
      "cStandard": "gnu11",
      "intelliSenseMode": "linux-gcc-x64"
    }
  ],
  "version": 4
}
```

Create `.vscode/launch.json`:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug RootStream",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/rootstream",
      "args": ["--service"],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}",
      "environment": [],
      "externalConsole": false,
      "MIMode": "gdb"
    }
  ]
}
```

---

## Code Guidelines

We follow a Linux-style C code approach with some project-specific conventions.

### Indentation

- 4 spaces (NO TABS)  
- Align continuation lines sensibly  

### Naming

```c
// Functions: lowercase with underscores
int rootstream_init(rootstream_ctx_t *ctx);

// Structs: _t suffix
typedef struct {
    int value;
} example_t;

// Constants: UPPERCASE
#define MAX_PEERS 16

// Enums: lowercase with descriptive names
typedef enum {
    PEER_CONNECTED,
    PEER_DISCONNECTED
} peer_state_t;
```

### Comments

Use structured comments for functions and complex logic:

```c
/*
 * Function header comment
 *
 * Explains what the function does, parameters, return value.
 * Use multi-line C-style comments for documentation.
 */
int example_function(int param) {
    /* Single-line explanation for complex logic */
    if (condition) {
        // Quick inline comment for non-obvious things
        do_something();
    }

    return 0;
}
```

**Good comments:**

```c
/*
 * We use Ed25519 instead of RSA because:
 * 1. Smaller keys (32 bytes vs 256+ bytes)
 * 2. Faster signing/verification
 * 3. No known weak curve vulnerabilities
 */
crypto_sign_keypair(pk, sk);
```

**Bad comments:**

```c
/* Increment i */
i++;  // DON'T STATE THE OBVIOUS
```

### Error Handling

```c
// GOOD: Informative errors
if (crypto_init() < 0) {
    fprintf(stderr, "ERROR: Crypto initialization failed\n");
    fprintf(stderr, "REASON: libsodium not properly installed\n");
    fprintf(stderr, "FIX: Install libsodium (see README)\n");
    return -1;
}

// BAD: Vague errors
if (crypto_init() < 0) {
    printf("error\n");  // USELESS!
    return -1;
}
```

### Memory Management

```c
// Always check allocations
uint8_t *buffer = malloc(size);
if (!buffer) {
    fprintf(stderr, "ERROR: Cannot allocate %zu bytes\n", size);
    return -1;
}

// Always free what you allocate
free(buffer);
buffer = NULL;  // Prevent use-after-free

// Use cleanup helpers where possible
void cleanup_ctx(rootstream_ctx_t *ctx) {
    if (ctx->buffer) {
        free(ctx->buffer);
        ctx->buffer = NULL;
    }
    // ... cleanup other resources
}
```

### Security

```c
// ALWAYS validate inputs
int process_packet(const uint8_t *data, size_t len) {
    if (!data || len == 0) {
        return -1;  // Fail safe
    }

    if (len > MAX_PACKET_SIZE) {
        fprintf(stderr, "WARNING: Packet too large, ignoring\n");
        return -1;  // Don't process malicious data
    }

    // ... process
}

// NEVER trust network data blindly
// BAD:
memcpy(dest, untrusted_input, len);

// GOOD:
memcpy(dest, untrusted_input, MIN(len, MAX_SIZE));
```

---

## Code Quality Checklist

Before submitting, ensure:

- [ ] Code compiles without warnings (`make`)  
- [ ] No memory leaks (`valgrind --leak-check=full ./rootstream`)  
- [ ] Static analysis passes (`make check`)  
- [ ] Formatted correctly (`make format`)  
- [ ] Error messages are informative  
- [ ] Functions have header comments  
- [ ] Complex logic has explanatory comments  
- [ ] Input validation on all external data  
- [ ] No obvious security issues (buffer overflows, etc.)  
- [ ] Tested on actual hardware  

---

## Commit Messages

We use a **conventional commit**-style format for clarity.

### Format

```text
<type>(<scope>): <subject>

<body>

<footer>
```

### Types

- `feat`   – New feature  
- `fix`    – Bug fix  
- `docs`   – Documentation changes  
- `style`  – Code formatting (no logic change)  
- `refactor` – Code restructuring (no behavior change)  
- `perf`   – Performance improvement  
- `test`   – Adding tests  
- `chore`  – Build process, dependencies, etc.

### Examples (Good)

```text
feat(encoder): add NVENC hardware encoding support

Implements direct NVENC API access for NVIDIA GPUs, bypassing
the inefficient VA-API → VDPAU wrapper. Reduces CPU usage from
12% to 5% on RTX 4070.

Tested on:
- RTX 4070: 1080p60 @ 5% CPU
- RTX 3060: 1080p60 @ 6% CPU
- GTX 1660: 1080p30 @ 8% CPU

Closes #42
```

```text
fix(network): prevent packet fragmentation on jumbo frames

UDP packets larger than MTU (1500 bytes) were causing
fragmentation and packet loss. Now we split large frames
into MTU-safe chunks (1400 bytes).

Fixes #78
```

```text
docs(README): clarify NVIDIA driver installation

Added section explaining why mesa-va-drivers should NOT be
installed on NVIDIA systems. Prevents common configuration
mistake.

Resolves confusion from #103, #127, #134
```

### Examples (Bad)

```text
fixed stuff
```

```text
Update crypto.c
```

```text
WIP
```

### Commit Frequency

- Prefer small, logical commits – one change per commit  
- Commit often – don’t accumulate giant 500+ line mixed changes  
- Each commit should build – don’t break `git bisect`  

---

## Pull Request Process

### Before Submitting

Update your branch:

```bash
git checkout main
git pull upstream main
git checkout your-feature-branch
git rebase main
```

Squash WIP commits (if needed):

```bash
git rebase -i main
# Mark WIP commits as 'squash' in the editor
```

Test thoroughly:

```bash
make clean
make DEBUG=1
make check
# Run on actual hardware if possible
```

Update documentation where appropriate:

- `README.md` for user-facing changes  
- Code comments for complex logic  
- `CHANGELOG.md` (if/when it exists)  

### PR Template (Suggested Content)

When opening a PR, include:

```md
## Description
Brief description of changes.

## Motivation
Why is this change needed?

## Changes
- Added X
- Fixed Y
- Refactored Z

## Testing
- [ ] Tested on Intel GPU
- [ ] Tested on AMD GPU
- [ ] Tested on NVIDIA GPU
- [ ] No memory leaks (valgrind)
- [ ] No new warnings
- [ ] Static analysis passes

## Checklist
- [ ] Code follows style guidelines
- [ ] Self-reviewed my code
- [ ] Commented complex logic
- [ ] Updated documentation
- [ ] No breaking changes (or documented)

## Related Issues
Closes #123  
Related to #456
```

### Review Process

- CI runs automatically (once set up).  
- Maintainer reviews within **1–7 days** where possible.  
- Address feedback:

```bash
# Make changes
git add .
git commit -m "fix: address review comments"
git push
```

- Approval – Maintainer approves.  
- Merge – Usually squash-merge to keep history clean.

### What We Look For

✅ **Good**

- Solves a real problem  
- Well-tested on hardware  
- Clear, readable code  
- Informative error messages  
- Proper error handling  
- No memory leaks  

❌ **Needs Work**

- Breaks existing functionality  
- Cryptic error messages  
- Memory leaks  
- Unsafe code (buffer overflows, etc.)  
- Un-commented complex logic  
- Not tested on hardware  

---

## Testing

### Manual Testing

**Basic workflow test:**

```bash
# Terminal 1: Start host
./rootstream host

# Terminal 2: Show QR code
./rootstream --qr

# Terminal 3: Connect client (when implemented)
./rootstream connect <code>
```

Verify:

- Video streaming works  
- Input events processed  
- No crashes  
- Clean shutdown  

**Memory leak test:**

```bash
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes \
         ./rootstream host

# Run for ~5 minutes, then Ctrl+C
# Target: "All heap blocks were freed -- no leaks are possible"
```

**Performance test:**

```bash
# Monitor CPU usage
htop

# Monitor GPU usage
intel_gpu_top      # Intel
radeontop          # AMD
nvidia-smi -l      # NVIDIA

# Target: < ~10% CPU for 1080p60 where possible
```

### Automated Testing (TODO)

We need contributors to help build:

- Unit tests (for core library pieces)  
- Integration tests  
- Fuzzing for packet parsing  
- CI/CD pipeline (GitHub Actions)

**Want to help?** Open an issue:  
> "I'd like to help with testing infrastructure"

---

## Priority Areas

### 🔴 Critical (v1.1)

**Client Implementation**

- VA-API decoder  
- Display output (SDL2 or DRM/KMS)  
- Input capture (keyboard/mouse)  
- Network receive and reassembly  

**Why this matters:**  
RootStream is currently host-only. A client is required for actual use.

**Skills needed:** C, VA-API, SDL2 or DRM/KMS, networking.

---

### 🟡 High Priority (v1.2)

**NVENC Support**

- Direct NVENC API (not VA-API wrapper)  
- Reduces NVIDIA CPU usage significantly  
- Better encoding quality

**Skills needed:** C, NVENC SDK, NVIDIA driver knowledge.

**Audio Streaming**

- ALSA direct capture  
- Opus encoding  
- A/V synchronization  
- Low-latency audio pipeline

**Skills needed:** C, ALSA, Opus codec, A/V sync.

**H.265/HEVC**

- Better compression (~50% bandwidth savings)  
- VA-API HEVC encoding  
- Decoder support

**Skills needed:** C, VA-API, HEVC/H.265 knowledge.

---

### 🟢 Nice to Have (v2.0+)

**Cross-Platform Clients**

- Windows client  
- macOS client  
- Android app  
- iOS app  

**Skills needed:** Platform-specific development, UI design.

**Advanced Features**

- HDR support  
- Multi-monitor streaming  
- Recording to file  
- Bandwidth adaptation  
- VR streaming  

---

### 📝 Documentation

**Needed:**

- Video tutorials (YouTube)  
- Setup guides for different distros  
- Troubleshooting database  
- Performance tuning guide  
- Security audit documentation  

**Skills needed:** Technical writing, video production.

---

## Communication

### Where to Discuss

**GitHub Issues**

- Bug reports  
- Feature requests  
- Design discussions  

**GitHub Discussions** (if/when enabled)

- General questions  
- Ideas and brainstorming  
- Show and tell  

**Discord** (if created)

- Real-time chat  
- Quick questions  
- Community building  

**IRC** (if needed)

- `#rootstream` on Libera.Chat (proposed)

### Response Times

We're all volunteers. Approximate targets:

- **Issues:** Response within 1–7 days  
- **PRs:** Review within 7–14 days  
- **Security:** First response within 24 hours  

**Urgent security issue?** See [Security](#security) below.

---

## Recognition

### Contributors

All contributors are credited in:

- `README.md` (Contributors section, when added)  
- Release notes  
- Git history (obviously!)  

### Significant Contributions

Major features/fixes may receive special recognition:

- Highlighted in release announcements  
- Social media shoutouts (if desired)  
- Listed in an `AUTHORS` file (future)  

### Becoming a Maintainer

Consistent, quality contributions over time may lead to:

- Merge/commit access  
- Issue triage permissions  
- Participation in architecture decisions  

**Typical criteria:**

- 5+ merged PRs  
- Good code quality and communication  
- Active for 3+ months  
- Community-minded attitude  

---

## Getting Help

### Stuck on Something?

**Before asking:**

1. Check existing issues.  
2. Read relevant code and comments.  
3. Search the documentation/README.  
4. Try debugging with GDB/valgrind/logging.

**When asking:**

- Show what you tried.  
- Include error messages.  
- Provide system info.  
- Be specific.

**Good question:**

```text
I'm implementing the VA-API decoder (issue #42) and getting
error -1 from vaMapBuffer(). I've verified:
- Surface is created successfully
- Context is initialized
- Same code works in vaapi_encoder.c

Error: "vaMapBuffer failed: invalid VASurface"

System: Arch Linux, Intel UHD 730, libva 2.20.0

Code: [link to branch]

Any ideas what I'm missing?
```

**Bad question:**

```text
decoder doesn't work help
```

---

## Security

**Do not open public GitHub issues for security vulnerabilities.**

If you discover a security issue:

1. Email the maintainer directly (see `SECURITY.md` or repository contact info).  
2. Include:
   - A clear description of the issue  
   - Steps to reproduce  
   - Any potential impact you’ve identified  

We will:

- Acknowledge your report as soon as reasonably possible.  
- Investigate the issue and work on a fix.  
- Coordinate disclosure timing with you when appropriate.  

In scope:

- Remote code execution  
- Unauthorized access to streams or keys  
- Cryptographic misuse  
- Privilege escalation via RootStream  

Not in scope:

- Misconfiguration of the host system (firewall, etc.)  
- Compromise of other software on the same host  
- Physical access attacks  

RootStream aims to use well-vetted cryptographic primitives via libsodium and a minimal attack surface.  
Security review and feedback are highly appreciated.

---

## Licensing

By contributing, you agree that your contributions will be licensed under the same license as the project (see `LICENSE` file).

If you cannot agree to this (e.g. due to employer IP policies), please discuss with the maintainer before submitting substantial work.

---

## Final Notes

### Philosophy Recap

RootStream is built on these principles:

- **User Freedom** – No accounts, no tracking, no lock-in.  
- **Security** – End-to-end encryption, no custom crypto.  
- **Simplicity** – Do one thing well, avoid scope creep.  
- **Performance** – Low latency, efficient resource usage.  
- **Quality** – Clear errors, good docs, robust behavior.

When making decisions or proposing changes, keep these in mind.

### Have Fun!

This is a community project. We’re here because we love:

- Linux gaming  
- Good software  
- Helping each other  
- Learning new things  

Don’t be intimidated! Every expert was once a beginner.  
Ask questions, experiment, break things (in your fork!), and learn.

---

## Thank You! 🎉

Every contribution, no matter how small, makes RootStream better. Whether you:

- Fixed a typo  
- Reported a bug  
- Answered someone's question  
- Wrote a feature  
- Tested on your hardware  

**You’re appreciated.**  
Thank you for being part of RootStream.

If you have questions about contributing, open a discussion or issue – we’re happy to help.  
Ready to contribute? Check the `good first issue` label!
