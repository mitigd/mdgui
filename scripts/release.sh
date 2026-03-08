#!/usr/bin/env sh
set -eu

VERSION="${1:-0.1.0}"
OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH="$(uname -m)"

case "$OS" in
  linux*) OS_NAME="linux" ;;
  darwin*) OS_NAME="macos" ;;
  *) OS_NAME="$OS" ;;
esac

ARTIFACT="mdgui-${VERSION}-${OS_NAME}-${ARCH}.tar.gz"
STAGE_DIR="mdgui-${VERSION}-${OS_NAME}-${ARCH}"

echo "Building release (ReleaseFast)..."
zig build -Doptimize=ReleaseFast

if [ -d "$STAGE_DIR" ]; then
  rm -rf "$STAGE_DIR"
fi
mkdir -p "$STAGE_DIR"

BIN_PATH="zig-out/bin/mdgui-demo"
if [ ! -f "$BIN_PATH" ]; then
  echo "Expected binary not found: $BIN_PATH" >&2
  exit 1
fi

cp "$BIN_PATH" "$STAGE_DIR/mdgui-demo"
cp README.md "$STAGE_DIR/README.md"

tar -czf "$ARTIFACT" "$STAGE_DIR"
rm -rf "$STAGE_DIR"

echo "Release artifact created: $ARTIFACT"
