# RootStream Quick Start Guide

## Installation (5 minutes)

### 1. Install Dependencies
```bash
sudo pacman -S base-devel libdrm libva
```

For NVIDIA users:
```bash
sudo pacman -S libva-vdpau-driver
```

### 2. Build and Install
```bash
cd rootstream
./install.sh
```

Or manually:
```bash
make
sudo make install
```

### 3. Setup Permissions
```bash
# Add yourself to video group
sudo usermod -a -G video $USER

# Load uinput module
sudo modprobe uinput
echo "uinput" | sudo tee /etc/modules-load.d/rootstream.conf

# Log out and back in for group changes
```

### 4. Test VA-API (Optional)
```bash
# Install va-utils
sudo pacman -S libva-utils

# Test
vainfo
```

Should show your GPU and supported profiles.

## Basic Usage

### Start Host (Gaming PC)
```bash
rootstream host
```

You should see:
```
╔═══════════════════════════════════════════════╗
║         RootStream v0.1.0                     ║
║  Native Linux Game Streaming - No BS          ║
╚═══════════════════════════════════════════════╝

🔍 Detecting displays...
✓ Selected: card0-HDMI-A-1 (1920x1080 @ 60 Hz)

✓ DRM capture initialized: 1920x1080 @ 60 Hz
✓ VA-API 1.20 initialized
✓ VA-API encoder ready: 1920x1080 @ 60 fps, 10000 kbps
✓ Network initialized on 0.0.0.0:9876 (UDP)
✓ Virtual input devices created

🎮 Host mode - waiting for client...
```

### Connect Client (Not Yet Implemented)
```bash
rootstream client 192.168.1.100
```

## Advanced Usage

### Select Specific Display

If you have multiple monitors:
```bash
# List displays first
rootstream host --list

# Select by index
rootstream host --display 1
```

### Custom Port
```bash
rootstream host --port 9999
```

### Run as Service
```bash
# Copy service file
sudo cp rootstream.service /etc/systemd/system/rootstream@.service

# Enable for your user
sudo systemctl enable rootstream@$USER

# Start
sudo systemctl start rootstream@$USER

# Check status
sudo systemctl status rootstream@$USER

# View logs
sudo journalctl -u rootstream@$USER -f
```

## Troubleshooting

### "Cannot open /dev/dri/card0"

**Problem:** Permission denied

**Solution:**
```bash
# Check if device exists
ls -l /dev/dri/

# Add to video group
sudo usermod -a -G video $USER

# Log out and back in
```

### "VA-API initialization failed"

**Problem:** No hardware encoder found

**Solution for NVIDIA:**
```bash
sudo pacman -S libva-vdpau-driver
```

**Test:**
```bash
vainfo
```

Should show supported profiles. If not, your GPU might not support VA-API.

### "No active displays found"

**Problem:** No connected displays

**Solution:**
```bash
# Check DRM devices
ls -l /dev/dri/

# Check display status
cat /sys/class/drm/card*/status

# Try different card
rootstream host --device /dev/dri/card1
```

### High Latency

**Checklist:**
- [ ] Using wired Ethernet (WiFi adds 5-15ms)
- [ ] VA-API working (not software encoding)
- [ ] No other network-heavy apps running
- [ ] Router QoS configured for gaming

**Measure latency:**
```bash
# On client (when implemented)
rootstream client HOST --show-stats
```

### Choppy Video

**Possible causes:**
1. Network congestion
2. CPU too slow
3. GPU encoder overloaded

**Solutions:**
```bash
# Lower resolution
rootstream host --resolution 720p

# Lower bitrate
rootstream host --bitrate 5000

# Lower framerate
rootstream host --fps 30
```

## Performance Tips

### Network Optimization

**Wired Ethernet:**
- Latency: 1-2ms
- Bandwidth: 1Gbps
- **Recommended**

**WiFi 6 (5GHz):**
- Latency: 3-8ms
- Bandwidth: 500+ Mbps
- Acceptable for 1080p60

**WiFi 5 (2.4GHz):**
- Latency: 10-20ms
- Bandwidth: 100 Mbps
- Not recommended

### Router Configuration

If you have QoS (Quality of Service):
```
Priority: High
Ports: 9876 (UDP)
Type: Gaming/Streaming
```

### GPU Selection

**Best:**
- Intel Arc (excellent VA-API)
- AMD RX 6000+ (RDNA2+)
- Recent Intel iGPU (11th gen+)

**Good:**
- AMD RX 5000 (RDNA)
- Intel iGPU (8th-10th gen)

**Acceptable:**
- NVIDIA (via VA-API wrapper)
- AMD GCN cards

## Monitoring Performance

### Check Statistics

While streaming, RootStream shows stats every 5 seconds:
```
📊 FPS: 60 | Bitrate: 10.2 Mbps | Frames: 12458/12450
```

**What this means:**
- **FPS:** Capture rate (should match display)
- **Bitrate:** Network usage
- **Frames:** Captured/Encoded (should be similar)

### Low FPS

If FPS is below display rate:

1. **Check CPU usage:**
```bash
   htop
```
   Should be <10% for rootstream

2. **Check GPU usage:**
```bash
   intel_gpu_top  # Intel
   radeontop      # AMD
   nvidia-smi     # NVIDIA
```

3. **Reduce load:**
   - Close other apps
   - Lower resolution/bitrate
   - Check for thermal throttling

### High Bitrate

If bitrate is too high for your network:
```bash
# Lower bitrate
rootstream host --bitrate 5000  # 5 Mbps

# Or lower quality
rootstream host --quality 75    # 0-100
```

## Next Steps

1. **Test locally first**
   - Run host
   - Verify capture working
   - Check network stats

2. **Test on LAN**
   - Connect from another PC
   - Verify low latency
   - Test input response

3. **Optimize settings**
   - Adjust bitrate for your network
   - Tune quality vs. latency
   - Find best resolution

4. **Report issues**
   - GitHub issues
   - Include logs
   - System info (GPU, network)

## Getting Help

**Documentation:**
- README.md - Overview
- ARCHITECTURE.md - Technical details
- This file - Quick start

**Common Issues:**
- Check closed issues on GitHub
- Search the documentation
- Ask in discussions

**Reporting Bugs:**
```bash
# Collect logs
rootstream host --verbose > rootstream.log 2>&1

# System info
uname -a
pacman -Q | grep -E 'libdrm|libva|mesa'
vainfo

# Attach to issue
```

---

**Ready to stream? Let's go!** 🎮
```bash
rootstream host
```
