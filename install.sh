#!/bin/sh

set -e

REPO="lenartkladnik/ascinfo"
BIN_NAME="ascinfo"
INSTALL_DIR="/usr/local/bin"

echo "Fetching latest $BIN_NAME release..."

URL=$(curl -s "https://api.github.com/repos/$REPO/releases/latest" |
  grep "browser_download_url" |
  grep "\"$BIN_NAME\"" |
  cut -d '"' -f 4)

if [ -z "$URL" ]; then
  echo "Error: could not find $BIN_NAME binary in the latest release."
  exit 1
fi

echo "Downloading $URL..."
curl -fL "$URL" -o "/tmp/$BIN_NAME"

echo "Installing to $INSTALL_DIR/$BIN_NAME (may require sudo)..."
sudo install -m 755 "/tmp/$BIN_NAME" "$INSTALL_DIR/$BIN_NAME"
rm "/tmp/$BIN_NAME"

echo "Successfully installed $BIN_NAME"
