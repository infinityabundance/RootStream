# RootStream KDE Plasma Client - Troubleshooting

## Common Issues

### Connection Issues

#### Cannot connect to peer

**Symptoms:**
- "Failed to connect to peer" error
- Connection times out

**Solutions:**

1. **Verify both devices are on the same network**
   ```bash
   ping <host-ip>
   ```

2. **Check firewall settings**
   ```bash
   # Allow UDP port 9876
   sudo ufw allow 9876/udp
   ```

3. **Verify RootStream code is correct**
   - Code format: `pubkey@hostname`
   - Example: `kXx7YqZ3...Qp9w==@gaming-pc`

4. **Try IP address instead of hostname**
   ```bash
   rootstream-kde-client --connect "192.168.1.100:9876"
   ```

#### Connection drops frequently

**Solutions:**

1. **Use wired ethernet instead of WiFi**
2. **Check network quality**
   ```bash
   mtr <host-ip>
   ```
3. **Lower bitrate** - Settings → Video → Bitrate
4. **Enable TCP fallback** - Check advanced settings

### Video Issues

#### Black screen / no video

**Symptoms:**
- Connected but screen remains black
- Placeholder message shown

**Solutions:**

1. **Verify host is streaming**
   - Check host is in "host mode"
   - Verify capture backend is working

2. **Check codec support**
   - Try different codec (Settings → Video → Codec)
   - H.264 is most compatible

3. **Check hardware decoder**
   ```bash
   vainfo  # Should show supported profiles
   ```
   Install VA-API drivers if not present:
   ```bash
   sudo pacman -S libva libva-intel-driver  # Intel
   sudo pacman -S libva mesa              # AMD
   ```

4. **Enable software fallback**
   - Client will automatically fall back to software decoding

5. **Check diagnostics**
   - Help → Diagnostics
   - Look for decoder errors

#### Video stuttering / low framerate

**Solutions:**

1. **Lower bitrate** - Settings → Video → Bitrate (try 5-8 Mbps)
2. **Check CPU usage** - Video decoding may be CPU-bound
3. **Close other applications**
4. **Use hardware acceleration**
   - Ensure VA-API is working: `vainfo`

#### Video artifacts / corruption

**Solutions:**

1. **Check network quality** - Packet loss causes artifacts
2. **Lower bitrate** - Reduces bandwidth requirements
3. **Try different codec** - Some codecs more resilient to packet loss
4. **Use wired connection**

### Audio Issues

#### No audio

**Symptoms:**
- Video plays but no audio
- Silent stream

**Solutions:**

1. **Verify audio is enabled**
   - Settings → Audio → Enable audio

2. **Check audio device**
   - Settings → Audio → Audio device
   - Try "Default" first

3. **Check PulseAudio/PipeWire status**
   ```bash
   # PulseAudio
   pulseaudio --check
   pulseaudio -D
   
   # PipeWire
   systemctl --user status pipewire
   systemctl --user start pipewire
   ```

4. **Check volume**
   ```bash
   pactl list sinks  # PulseAudio
   wpctl status      # PipeWire
   ```

5. **Test audio device**
   ```bash
   speaker-test -t wav -c 2
   ```

#### Audio stuttering / crackling

**Solutions:**

1. **Check buffer size** - Increase audio buffer in settings
2. **Close other audio applications**
3. **Check CPU usage** - Audio decoding may be CPU-bound
4. **Try different audio backend**
   - PulseAudio vs. PipeWire vs. ALSA

#### Audio/video out of sync

**Solutions:**

1. **Lower latency** - Reduce bitrate, use wired connection
2. **Check frame drops** - Status bar shows FPS
3. **Restart client** - Audio sync may recover

### Input Issues

#### Input not working

**Symptoms:**
- Mouse/keyboard input not reaching host
- No response to input

**Solutions:**

1. **Check input mode**
   - Settings → Input → Input mode
   - Try both "uinput" and "xdotool"

2. **Check permissions** (for uinput)
   ```bash
   sudo usermod -a -G input $USER
   # Log out and back in
   ```

3. **Install xdotool** (for xdotool mode)
   ```bash
   sudo pacman -S xdotool
   ```

4. **Check focus** - Window must have focus for input

#### High input lag

**Solutions:**

1. **Lower bitrate** - Reduces overall latency
2. **Use wired connection**
3. **Check network latency**
   ```bash
   ping <host-ip>  # Should be <5ms on LAN
   ```
4. **Close background applications**

### Performance Issues

#### High CPU usage

**Solutions:**

1. **Enable hardware decoding**
   - Install VA-API drivers
   - Check: `vainfo`

2. **Lower resolution** - Ask host to lower capture resolution
3. **Lower framerate** - Ask host to lower FPS
4. **Use H.264 codec** - Most efficient

#### High memory usage

**Solutions:**

1. **Restart application** - May have memory leak
2. **Check for updates** - Bug may be fixed
3. **Report issue** - With AI logging enabled

### Build/Installation Issues

#### CMake cannot find Qt6

**Solution:**
```bash
export CMAKE_PREFIX_PATH=/usr/lib/qt6
cmake ..
```

#### Missing dependencies

**Solution:**
```bash
# Arch Linux
sudo pacman -S qt6-base qt6-declarative libsodium opus

# Ubuntu
sudo apt install qt6-base-dev qt6-declarative-dev libsodium-dev libopus-dev
```

#### libRootStream not found

**Solution:**
Build and install libRootStream first:
```bash
cd ../../
make
sudo make install
cd clients/kde-plasma-client
```

## Getting Help

### Enable AI Logging

For detailed diagnostics, enable AI logging:

```bash
rootstream-kde-client --ai-logging 2>&1 | tee client.log
```

This creates structured logs useful for debugging.

### Check Diagnostics

In the application:
1. Help → Diagnostics
2. Copy output
3. Include in bug report

### Report Issues

1. Go to https://github.com/infinityabundance/RootStream/issues
2. Create new issue
3. Include:
   - OS and version
   - Qt version (`qmake6 --version`)
   - Hardware (GPU, CPU)
   - Steps to reproduce
   - AI logging output (if relevant)
   - Diagnostics output

### Debug Mode

Run with debug output:

```bash
QT_LOGGING_RULES="*=true" rootstream-kde-client --ai-logging
```

### Still Need Help?

- Check existing GitHub issues
- Join community discussions
- Contact developers

## Known Issues

### Current Limitations

1. **Video rendering not yet implemented**
   - Placeholder shown instead of stream
   - Planned for next release

2. **Audio playback not yet implemented**
   - No audio output yet
   - Planned for next release

3. **Input injection not yet implemented**
   - Cannot control remote system yet
   - Planned for next release

4. **Peer discovery incomplete**
   - mDNS discovery partially implemented
   - Manual peer entry works

These are work-in-progress features and will be completed in future releases.
