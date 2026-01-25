# RootStream - Native Linux Game Streaming
# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include
LDFLAGS = -ldrm -lva -lva-drm -lpthread

# Source files
SRCS = src/main.c \
       src/drm_capture.c \
       src/vaapi_encoder.c \
       src/network.c \
       src/input.c

OBJS = $(SRCS:.c=.o)
TARGET = rootstream

# Build target
all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "🔗 Linking $@..."
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Build complete!"

%.o: %.c
	@echo "🔨 Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Install
install: $(TARGET)
	@echo "📦 Installing to /usr/local/bin..."
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "✓ Installed!"

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)
	@echo "✓ Uninstalled"

# Clean
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "✓ Cleaned"

# Dependencies check
deps:
	@echo "Checking dependencies..."
	@command -v gcc >/dev/null 2>&1 || { echo "❌ gcc not found"; exit 1; }
	@pkg-config --exists libdrm || { echo "❌ libdrm not found"; exit 1; }
	@pkg-config --exists libva || { echo "❌ libva not found"; exit 1; }
	@echo "✓ All dependencies found"

# Help
help:
	@echo "RootStream Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the project (default)"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  uninstall - Remove from system"
	@echo "  clean     - Remove build artifacts"
	@echo "  deps      - Check dependencies"
	@echo "  help      - Show this help"

.PHONY: all install uninstall clean deps help
