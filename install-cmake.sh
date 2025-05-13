#!/usr/bin/env bash
# This script installs the latest supported version of cmake to ~/cmake, then adds
# ~/cmake/bin to the PATH. This script is provided as a convenience; you can feel free
# to install cmake however you like instead.
set -e

# Establish desired version and URL to official install script
CMAKE_VERSION="4.0.2"
CMAKE_SH_FILENAME="cmake-$CMAKE_VERSION-linux-x86_64.sh"
CMAKE_SH_URL="https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/$CMAKE_SH_FILENAME"

# Establish where cmake should be installed for the current user
CMAKE_DIR="$HOME/cmake"
CMAKE_BIN_DIR="$CMAKE_DIR/bin"

# Don't clobber an existing cmake install
if [ -d "$CMAKE_DIR" ]; then
  echo "ERROR: $CMAKE_DIR already exists!"
  exit 1
fi
mkdir -p "$CMAKE_DIR"

# Download and run the cmake install script, then delete it
wget "$CMAKE_SH_URL"
chmod +x "./$CMAKE_SH_FILENAME"
"./$CMAKE_SH_FILENAME" --prefix="$CMAKE_DIR" --exclude-subdir --skip-license
rm "./$CMAKE_SH_FILENAME"

# Add the cmake bin directory to the user's path in .profile
if [[ ":$PATH:" != *":$CMAKE_BIN_DIR:"* ]]; then
  # Default to .bashrc, but use .zshrc instead if the user's preferred shell is zsh
  SHELL_NAME=$(basename "$SHELL")
  SHELL_CONFIG_FILE="$HOME/.bashrc"
  if [ "$SHELL_NAME" == "zsh" ]; then
    SHELL_CONFIG_FILE="$HOME/.zshrc"
  fi

  # Add an export PATH=... entry to the target config file if necessary
  if ! grep -Fxq "export PATH=\"$CMAKE_BIN_DIR:\$PATH\"" "$SHELL_CONFIG_FILE"; then
    echo -e "\nexport PATH=\"$CMAKE_BIN_DIR:\$PATH\"" >> "$SHELL_CONFIG_FILE"
    echo "Added $CMAKE_BIN_DIR to PATH in $SHELL_CONFIG_FILE."
  else
    echo "$CMAKE_BIN_DIR is already added to PATH in $SHELL_CONFIG_FILE."
  fi
  echo "Please reload your shell by running: exec $SHELL_NAME"
else
  echo "$CMAKE_BIN_DIR is already in PATH."
fi
