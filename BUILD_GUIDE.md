# Build Guide for StreamUP BBCode Plugin
## Prerequisites
### Windows
- Visual Studio 2022 (Community, Professional, or Enterprise)
- CMake 3.28 or higher
- OBS Studio source code (v31.1.1 recommended)
- Git
### Linux
- GCC 11+ or Clang 12+
- CMake 3.28+
- OBS Studio dev packages
- Git
### macOS
- Xcode 14+
- CMake 3.28+
- OBS Studio source code
- Git
## Step 1: Set Up OBS Development Environment
### Windows Setup
1. Clone OBS Studio repository:
   `ash
   git clone --recursive https://github.com/obsproject/obs-studio.git
   cd obs-studio
   `
2. Build OBS (this takes a while):
   `ash
   cmake --preset windows-build
   cmake --build build_x64 --config Release
   `
3. Set the OBS_SOURCE_DIR environment variable:
   `ash
   setx OBS_SOURCE_DIR "C:\path\to\obs-studio"
   `
### Linux Setup
`ash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install -y build-essential cmake git libobs-dev libqt6gui6
# Set environment
export OBS_SOURCE_DIR=/path/to/obs-studio
`
### macOS Setup
`ash
# Install Homebrew if needed, then:
brew install cmake obs-studio
# Set environment
export OBS_SOURCE_DIR=/path/to/obs-studio
`
## Step 2: Build the Plugin
### Windows
`ash
cd L:\SUP_BB_Code_New
# Configure for Windows x64
cmake --preset windows-x64
# Build Release
cmake --build build_x64 --config Release
# Output: build_x64\Release\sup-bbcode.dll
`
### Linux
`ash
cd ~/SUP_BB_Code_New
# Configure
cmake --preset linux-x86_64
# Build
cmake --build build --config Release
# Output: build/Release/sup-bbcode.so
`
### macOS
`ash
cd ~/SUP_BB_Code_New
# Configure
cmake --preset macos-universal
# Build
cmake --build build --config Release
# Output: build/Release/sup-bbcode.so
`
## Step 3: Install Plugin
### Windows
`ash
# Plugin directory
%APPDATA%\obs-studio\plugins\sup-bbcode\bin\64bit\
# Data directory
%APPDATA%\obs-studio\plugins\sup-bbcode\data\
# Copy files
Copy-Item "build_x64\Release\sup-bbcode.dll" -Destination "C:\Users\tnpot\AppData\Roaming\obs-studio\plugins\sup-bbcode\bin\64bit\"
Copy-Item "data\*" -Destination "C:\Users\tnpot\AppData\Roaming\obs-studio\plugins\sup-bbcode\data\" -Recurse
`
### Linux
`ash
# Plugin directory
~/.config/obs-studio/plugins/sup-bbcode/bin/64bit/
# Data directory
~/.config/obs-studio/plugins/sup-bbcode/data/
# Copy files
cp build/Release/sup-bbcode.so ~/.config/obs-studio/plugins/sup-bbcode/bin/64bit/
cp -r data/* ~/.config/obs-studio/plugins/sup-bbcode/data/
`
### macOS
`ash
# Plugin directory
~/Library/Application\ Support/obs-studio/plugins/sup-bbcode/bin/
# Data directory
~/Library/Application\ Support/obs-studio/plugins/sup-bbcode/data/
# Copy files
cp build/Release/sup-bbcode.so ~/Library/Application\ Support/obs-studio/plugins/sup-bbcode/bin/
cp -r data/* ~/Library/Application\ Support/obs-studio/plugins/sup-bbcode/data/
`
## Step 4: Verify Installation
1. Launch OBS Studio
2. Go to Tools > BBCode Help or check if text source has BBCode options
3. If installed correctly, you should see the BBCode formatting options
## Troubleshooting
### Plugin Not Loading
- Check OBS logs: %APPDATA%\obs-studio\logs\ (Windows)
- Verify plugin file in correct directory
- Check OBS version compatibility (requires v31.1.0+)
### Build Errors
- Ensure OBS_SOURCE_DIR is set correctly
- Run cmake --preset windows-x64 again (regenerate)
- Check CMakeLists.txt source file paths
- Verify all dependencies are installed
### Missing Frontend API
- Install obs-frontend-api development files
- Set ENABLE_FRONTEND_API=ON in CMake
- Rebuild plugin
## Debug Build
To create a debug build with symbols:
`ash
# Windows
cmake --preset windows-x64
cmake --build build_x64 --config Debug
# Linux/macOS
cmake --preset linux-x86_64  # or macos-universal
cmake --build build --config Debug
`
Debug builds are larger but allow for breakpoint debugging in Visual Studio.
## Environment Variables
| Variable | Value | Purpose |
|----------|-------|---------|
| OBS_SOURCE_DIR | Path to OBS source | Build against OBS source |
| OBS_DEPS_PATH | Path to obs-deps | Additional dependencies |
| CMAKE_PREFIX_PATH | OBS build directory | CMake package discovery |
## Additional Resources
- OBS Plugin Development: https://obsproject.com/wiki/Plugins-Guide
- OBS Frontend API: https://github.com/obsproject/obs-studio/tree/master/UI/obs-frontend-api
- CMake Documentation: https://cmake.org/documentation/
- StreamUP Documentation: https://docs.streamup.tips
## Support
For build issues or questions:
- 📄 Documentation: https://docs.streamup.tips
- 💬 Discord: https://discord.streamup.tips
- 🐛 GitHub Issues: https://github.com/StreamUPTips/SUP_BB_Code/issues
---
Last Updated: March 24, 2026
