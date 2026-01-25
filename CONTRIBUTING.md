Contributing to RootStream 🚀
First off, thank you for considering contributing to RootStream! This project was born from frustration with broken streaming solutions on Linux, and every contribution helps make it better.
╔════════════════════════════════════════════════╗
║  All Contributions Welcome!                    ║
║  • Bug reports                                 ║
║  • Feature requests                            ║
║  • Documentation improvements                  ║
║  • Code contributions                          ║
║  • Testing on different hardware               ║
╚════════════════════════════════════════════════╝
Table of Contents

Code of Conduct
Ways to Contribute
Getting Started
Development Setup
Code Guidelines
Commit Messages
Pull Request Process
Testing
Priority Areas
Communication
Recognition


Code of Conduct
Our Pledge
We're here to build great software and help each other. Be kind, be respectful, be constructive.
Expected Behavior

Be welcoming - Everyone was a beginner once
Be respectful - Disagree with ideas, not people
Be constructive - Criticism should include suggestions
Be patient - We're all volunteers with day jobs
Give credit - Acknowledge others' contributions

Unacceptable Behavior

Harassment, discrimination, or personal attacks
Trolling, insulting comments, or unconstructive criticism
Publishing others' private information
Any conduct inappropriate in a professional setting

Violations? Contact the maintainers. We'll handle it privately and professionally.

Ways to Contribute
🐛 Bug Reports
Found a bug? Excellent! Please open an issue with:
markdown**Description:**
Clear description of the bug

**Steps to Reproduce:**
1. Start RootStream with...
2. Click on...
3. See error

**Expected Behavior:**
What should happen

**Actual Behavior:**
What actually happens

**System Information:**
- OS: Arch Linux
- Kernel: 6.7.1
- GPU: Intel UHD 730
- RootStream version: 1.0.0

**Logs:**
```
[paste relevant logs here]
```

**Screenshots:**
[if applicable]
Pro tip: Run with verbose logging:
bashrootstream --service --verbose 2>&1 | tee rootstream.log
💡 Feature Requests
Have an idea? Open an issue with:

Use case - What problem does this solve?
Proposed solution - How would it work?
Alternatives considered - What else did you think about?
Willingness to implement - Can you code it yourself?

📚 Documentation
Documentation is just as important as code! Ways to help:

Fix typos or clarify confusing sections
Add more examples
Translate documentation (see i18n roadmap)
Write tutorials or guides
Improve code comments

🧪 Testing
Don't code? You can still help!

Test on different hardware (especially AMD/NVIDIA GPUs)
Try edge cases and report results
Verify fixes in pull requests
Test on different distros (Ubuntu, Fedora, etc.)

🎨 Design

UI/UX improvements for tray app
Icon design (we need better icons!)
QR code styling
Marketing materials


Getting Started
Prerequisites
Before you start coding, ensure you have:
```
bash# Arch Linux
sudo pacman -S base-devel git libdrm libva gtk3 libsodium qrencode libpng
```

# Development tools
```
sudo pacman -S gdb valgrind clang-tools-extra cppcheck
```

# Documentation tools (optional)
```

sudo pacman -S doxygen graphviz
Fork and Clone
bash# Fork on GitHub, then:
git clone https://github.com/YOUR_USERNAME/rootstream.git
cd rootstream
```

# Add upstream remote
```
git remote add upstream https://github.com/yourusername/rootstream.git
Build and Test
bash# Build with debug symbols
make DEBUG=1
```

# Run tests (when available)
```
make test

# Check for issues
make check
```

Create a Branch
```
bash# Update your fork
git checkout main
git pull upstream main
```

# Create feature branch
```
git checkout -b feature/awesome-feature
```

# Or fix branch
```
git checkout -b fix/bug-description
```
Development Setup
Recommended Tools
Editor/IDE:

VSCode with C/C++ extension
CLion (JetBrains)
Neovim with LSP (clangd)
Any editor with clangd support

Debugger:
```
bash# Run with GDB
gdb ./rootstream
(gdb) run --service
```
# Or with valgrind for memory leaks
```
valgrind --leak-check=full ./rootstream --service
Static Analysis:
bash# Check code quality
make check
```

# Format code (uses clang-format)
```
make format
VSCode Configuration
Create .vscode/c_cpp_properties.json:
json{
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
Create .vscode/launch.json:
json{
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
Code Guidelines
Code Style
We follow Linux kernel style with minor modifications:
Indentation:

4 spaces (NO TABS)
Align continuation lines sensibly

Naming:
```
c// Functions: lowercase with underscores
int rootstream_init(rootstream_ctx_t *ctx);

// Structs: lowercase_t suffix
typedef struct {
    int value;
} example_t;

// Constants: UPPERCASE
#define MAX_PEERS 16

// Enums: lowercase with prefix
typedef enum {
    PEER_CONNECTED,
    PEER_DISCONNECTED
} peer_state_t;
Comments:
c/*
 * Function header comment
 * 
 * Explains what the function does, parameters, return value.
 * Use multi-line C-style comments for documentation.
 */
int example_function(int param) {
    /* Single-line explanation for complex logic */
    if (condition) {
        // Quick inline comment for obvious things
        do_something();
    }
    
    return 0;
}
```
Good Comments:
```
c/* 
 * We use Ed25519 instead of RSA because:
 * 1. Smaller keys (32 bytes vs 256+ bytes)
 * 2. Faster signing/verification
 * 3. No weak curve vulnerabilities
 */
crypto_sign_keypair(pk, sk);
Bad Comments:
c/* Increment i */
i++;  // DON'T STATE THE OBVIOUS
Error Handling:
c// GOOD: Informative errors
if (crypto_init() < 0) {
    fprintf(stderr, "ERROR: Crypto initialization failed\n");
    fprintf(stderr, "REASON: libsodium not properly installed\n");
    fprintf(stderr, "FIX: Run 'sudo pacman -S libsodium'\n");
    return -1;
}

// BAD: Vague errors
if (crypto_init() < 0) {
    printf("error\n");  // USELESS!
    return -1;
}
Memory Management:
c// Always check allocations
uint8_t *buffer = malloc(size);
if (!buffer) {
    fprintf(stderr, "ERROR: Cannot allocate %zu bytes\n", size);
    return -1;
}

// Always free what you allocate

free(buffer);
buffer = NULL;  // Prevent use-after-free

// Use RAII pattern where possible
void cleanup_ctx(rootstream_ctx_t *ctx) {
    if (ctx->buffer) {
        free(ctx->buffer);
        ctx->buffer = NULL;
    }
    // ... cleanup other resources
}
Security:
c// ALWAYS validate inputs
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

// NEVER trust network data
memcpy(dest, untrusted_input, len);  // BAD!
memcpy(dest, untrusted_input, MIN(len, MAX_SIZE));  // GOOD
```

### Code Quality Checklist

Before submitting, ensure:

- [ ] Code compiles without warnings (`make`)
- [ ] No memory leaks (`valgrind --leak-check=full ./rootstream`)
- [ ] Static analysis passes (`make check`)
- [ ] Formatted correctly (`make format`)
- [ ] Error messages are informative
- [ ] Functions have header comments
- [ ] Complex logic has explanatory comments
- [ ] Input validation on all external data
- [ ] No security vulnerabilities (buffer overflows, etc.)
- [ ] Tested on actual hardware

---

## Commit Messages

### Format
```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code formatting (no logic change)
- `refactor`: Code restructuring (no behavior change)
- `perf`: Performance improvement
- `test`: Adding tests
- `chore`: Build process, dependencies, etc.

### Examples

**Good:**
```
feat(crypto): add NVENC hardware encoding support

Implements direct NVENC API access for NVIDIA GPUs, bypassing
the inefficient VA-API → VDPAU wrapper. Reduces CPU usage from
12% to 5% on RTX 4070.

Tested on:
- RTX 4070: 1080p60 @ 5% CPU
- RTX 3060: 1080p60 @ 6% CPU
- GTX 1660: 1080p30 @ 8% CPU

Closes #42
```
```
fix(network): prevent packet fragmentation on jumbo frames

UDP packets larger than MTU (1500 bytes) were causing
fragmentation and packet loss. Now we split large frames
into MTU-safe chunks (1400 bytes).

Fixes #78
```
```
docs(README): clarify NVIDIA driver installation

Added section explaining why mesa-va-drivers should NOT be
installed on NVIDIA systems. Prevents common configuration
mistake.

Resolves confusion from #103, #127, #134
```

**Bad:**
```
fixed stuff
```
```
Update crypto.c
```
```
WIP
Commit Frequency

Small, logical commits - One change per commit
Commit often - Don't accumulate 500 line changes
Each commit should build - Don't break bisect


Pull Request Process
Before Submitting

Update your branch:

bash   git checkout main
   git pull upstream main
   git checkout your-feature-branch
   git rebase main

Squash WIP commits (if needed):

bash   git rebase -i main
   # Mark commits as 'squash' in editor

Test thoroughly:

bash   make clean
   make DEBUG=1
   make check
   # Run on actual hardware

Update documentation:

README.md if user-facing
Code comments for complex logic
CHANGELOG.md (if exists)



PR Template
When opening a PR, include:
markdown## Description
Brief description of changes

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
Review Process

CI runs automatically (when set up)
Maintainer reviews within 1-7 days
Address feedback:

bash   # Make changes
   git add .
   git commit -m "fix: address review comments"
   git push

Approval - Maintainer approves
Merge - We'll merge (usually squash merge)

What We Look For
✅ Good:

Solves real problem
Well-tested on hardware
Clear, readable code
Informative error messages
Proper error handling
No memory leaks

❌ Needs Work:

Breaks existing functionality
Cryptic error messages
Memory leaks
Unsafe code (buffer overflows, etc.)
Uncommented complex logic
Not tested on hardware


Testing
Manual Testing
Basic workflow test:
bash# Terminal 1: Start host
./rootstream host

# Terminal 2: Show QR code
./rootstream --qr

# Terminal 3: Connect client (when implemented)
./rootstream connect <code>

# Verify:
# - Video streaming works
# - Input events processed
# - No crashes
# - Clean shutdown
Memory leak test:
bashvalgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes \
         ./rootstream host

# Run for 5 minutes, then Ctrl+C
# Should show "All heap blocks were freed -- no leaks are possible"
Performance test:
bash# Monitor CPU usage
htop

# Monitor GPU usage
intel_gpu_top  # Intel
radeontop      # AMD
nvidia-smi -l  # NVIDIA

# Target: <10% CPU for 1080p60
```

### Automated Testing (TODO)

We need contributors to help build:

- Unit tests (check library)
- Integration tests
- Fuzzing for packet parsing
- CI/CD pipeline (GitHub Actions)

**Want to help?** Open an issue: "I'd like to help with testing infrastructure"

---

## Priority Areas

### 🔴 Critical (v1.1)

**Client Implementation:**
- VA-API decoder
- Display output (SDL2 or DRM/KMS)
- Input capture (keyboard/mouse)
- Network receive and reassembly

**Why this matters:** RootStream is currently host-only. Client is needed for actual use!

**Skills needed:** C, VA-API, SDL2 or DRM/KMS

---

### 🟡 High Priority (v1.2)

**NVENC Support:**
- Direct NVENC API (not VA-API wrapper)
- Reduces NVIDIA CPU usage from 12% → 5%
- Better quality encoding

**Skills needed:** C, NVENC SDK, NVIDIA driver knowledge

**Audio Streaming:**
- ALSA direct capture
- Opus encoding
- A/V synchronization
- Low-latency pipeline

**Skills needed:** C, ALSA, Opus codec, audio/video sync

**H.265/HEVC:**
- Better compression (50% bandwidth savings)
- VA-API HEVC encoding
- Decoder support

**Skills needed:** C, VA-API, H.265 knowledge

---

### 🟢 Nice to Have (v2.0+)

**Cross-Platform Clients:**
- Windows client
- macOS client
- Android app
- iOS app

**Skills needed:** Platform-specific development, UI design

**Advanced Features:**
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

**Skills needed:** Technical writing, video production

---

## Communication

### Where to Discuss

**GitHub Issues:**
- Bug reports
- Feature requests
- Design discussions

**GitHub Discussions:**
- General questions
- Ideas and brainstorming
- Show and tell

**Discord** (if created):
- Real-time chat
- Quick questions
- Community building

**IRC** (if needed):
- #rootstream on Libera.Chat

### Response Times

We're all volunteers! Expect:
- **Issues:** Response within 1-7 days
- **PRs:** Review within 7-14 days
- **Security:** Response within 24 hours

**Urgent security issue?** Email directly (provide in SECURITY.md)

---

## Recognition

### Contributors

All contributors are credited in:
- README.md (Contributors section)
- Release notes
- Git history (obviously!)

### Significant Contributions

Major features/fixes get special recognition:
- Highlighted in release announcements
- Social media shoutouts (if desired)
- Listed in AUTHORS file

### Becoming a Maintainer

Consistent, quality contributions over time may lead to:
- Merge/commit access
- Issue triage permissions
- Architecture decisions

**Criteria:**
- 5+ merged PRs
- Good code quality
- Active for 3+ months
- Community-minded

---

## Getting Help

### Stuck on Something?

**Before asking:**
1. Check existing issues
2. Read relevant code comments
3. Search documentation
4. Try debugging with GDB

**When asking:**
- Show what you tried
- Include error messages
- Provide system info
- Be specific

**Good question:**
```
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
```
decoder doesn't work help

Final Notes
Philosophy
RootStream is built on these principles:

User Freedom - No accounts, no tracking, no lock-in
Security - End-to-end encryption, audited crypto
Simplicity - Do one thing well, avoid scope creep
Performance - Low latency, low resource usage
Quality - Excellent error messages, good documentation

When in doubt, ask: "Does this align with these principles?"
Have Fun!
This is a community project. We're here because we love:

Linux gaming
Good software
Helping each other
Learning new things

Don't be intimidated! Every expert was once a beginner. Ask questions, experiment, break things (in your fork!), and learn.

---

Thank You! 🎉
Every contribution, no matter how small, makes RootStream better. Whether you:

Fixed a typo
Reported a bug
Answered someone's question
Wrote a feature
Tested on your hardware

You're awesome. Thank you for being part of this!

Questions about contributing? Open a discussion or issue. We're happy to help!
Ready to contribute? Check the good first issue label!
