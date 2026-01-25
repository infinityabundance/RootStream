# Maintainer: RootStream Team
pkgname=rootstream
pkgver=0.1.0
pkgrel=1
pkgdesc="Native Linux game streaming - direct kernel access, no compositor BS"
arch=('x86_64')
url="https://github.com/yourusername/rootstream"
license=('MIT')
depends=('libdrm' 'libva' 'glibc')
makedepends=('gcc' 'make')
optdepends=(
    'libva-intel-driver: Intel GPU support'
    'libva-mesa-driver: AMD GPU support'
    'libva-vdpau-driver: NVIDIA GPU support via VDPAU wrapper'
    'mesa-vdpau: Mesa VDPAU drivers'
)
source=("$pkgname-$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$srcdir/$pkgname-$pkgver"
    make
}

package() {
    cd "$srcdir/$pkgname-$pkgver"
    
    # Install binary
    install -Dm755 rootstream "$pkgdir/usr/bin/rootstream"
    
    # Install systemd service
    install -Dm644 rootstream.service "$pkgdir/usr/lib/systemd/system/rootstream@.service"
    
    # Install documentation
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
    
    # Install license
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}

post_install() {
    echo "RootStream has been installed!"
    echo ""
    echo "Setup:"
    echo "  1. Add yourself to the 'video' group:"
    echo "     sudo usermod -a -G video \$USER"
    echo "  2. Log out and back in"
    echo "  3. Load uinput module:"
    echo "     sudo modprobe uinput"
    echo "     echo 'uinput' | sudo tee /etc/modules-load.d/rootstream.conf"
    echo ""
    echo "Usage:"
    echo "  rootstream host           # Start streaming"
    echo "  rootstream client HOST    # Connect to host"
    echo ""
    echo "For more info: /usr/share/doc/rootstream/README.md"
}

post_upgrade() {
    post_install
}
```

---

## 11. License

**File: `LICENSE`**
```
MIT License

Copyright (c) 2026 RootStream Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 12. Git Ignore

**File: `.gitignore`**
```
# Build artifacts
*.o
rootstream
*.a
*.so

# Editor files
*.swp
*.swo
*~
.vscode/
.idea/

# OS files
.DS_Store
Thumbs.db

# Logs
*.log

# Temporary files
*.tmp
