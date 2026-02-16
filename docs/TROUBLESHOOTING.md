# RootStream Troubleshooting

This guide focuses on **fast, actionable diagnostics**. Each section explains
what went wrong, why it happens, where to look, and how to fix it.

## Client cannot decode

**Symptoms**
- Client connects but shows no video.
- Logs mention decode/decoder failures or invalid bitstream.

**Likely causes**
- Missing VA-API/NVDEC drivers on the client.
- The client GPU does not support the codec profile.
- Bitstream is truncated due to packet loss or MTU mismatch.

**What to check**
- Run `vainfo` on Intel/AMD clients (or `nvidia-smi` + NVDEC capability).
- Verify that packet loss is low on the network path.

**Fix**
- Install the correct VA-API/NVDEC packages for your GPU.
- Lower bitrate or force a lower resolution to reduce packet loss.
- Confirm that both peers run matching RootStream versions.

## Black screen / no frames

**Symptoms**
- Host is running, client connects, but the screen stays black.

**Likely causes**
- DRM capture failed (no active framebuffer or missing permissions).
- Selected display index is invalid.
- Host compositor or DRM device is unavailable.

**What to check**
- Run `rootstream host --display N` and confirm the display list.
- Check `/dev/dri` permissions and whether the active GPU is present.

**Fix**
- Ensure the user owns the DRM device or add to the `video` group.
- Use a valid display index.
- On hybrid GPUs, ensure the active output is on the selected device.

## Input not working

**Symptoms**
- Video streams but keyboard/mouse actions have no effect on the host.

**Likely causes**
- uinput device creation failed on the host.
- Missing permissions for `/dev/uinput`.

**What to check**
- Run RootStream with sufficient permissions to access `/dev/uinput`.
- Confirm the `uinput` kernel module is loaded.

**Fix**
- Load the module: `sudo modprobe uinput`.
- Add udev rules for persistent access to `/dev/uinput`.

## Pairing or peer mismatch

**Symptoms**
- You connect to a peer but want to verify it is the correct device.

**What to check**
- Compare the **device fingerprint** shown on both devices.

**Fix**
- If fingerprints do not match, stop and re-pair using the correct RootStream code.

## Private key permissions warning

**Symptoms**
- Startup warns that private key permissions are too open.

**Likely causes**
- Config directory or key file permissions were changed manually.

**Fix**
- Run: `chmod 600 ~/.config/rootstream/identity.key`

## Build failures (dependencies)

**Symptoms**
- Build stops with missing headers or pkg-config errors.

**Fix**
- GTK3: install `libgtk-3-dev` (Debian/Ubuntu) or `gtk3-devel` (Fedora).
- libva: install `libva-dev` + `libva-drm` for VA-API.
- libsodium: install `libsodium-dev`.
- libqrencode/libpng: install `libqrencode-dev` and `libpng-dev`.

**Dependency-free build**
```
make HEADLESS=1 NO_CRYPTO=1 NO_QR=1 NO_DRM=1
```
This disables GUI, crypto, QR, and DRM for build troubleshooting only.
