# StreamUP BBCode Macros Plugin
A powerful OBS Studio plugin that enables advanced BBCode text rendering and effects for text sources.
## Features
- **BBCode Text Rendering**: Format text with tags like [b]bold[/b], [i]italic[/i], [u]underline[/u]
- **Text Effects**: Color, shadow, glow, and advanced styling options
- **Macros System**: Save and reuse custom BBCode tag combinations
- **Text Alignment**: Control text positioning (left, center, right)
- **Animation Effects**: Dynamic text animations and transitions
- **Image & Emote Support**: Import custom images and emotes (Twitch compatible)
- **Help Reference**: Built-in BBCode tag reference window
## Project Structure
\\\
src/
├── plugin-main.cpp           # Main plugin entry point
├── bbcode-source.h           # Source structure definitions
├── bbcode-source.cpp         # OBS source registration
├── bbcode-source-new.cpp     # Alternative source implementation
├── bbcode-parser.cpp         # BBCode parsing logic
├── bbcode-renderer.cpp       # Text rendering engine
├── bbcode-colors.cpp         # Color palette management
├── bbcode-macros.cpp         # Macro system and editor UI
├── bbcode-help.cpp           # Help window implementation
└── plugin-support.h          # Support utilities
data/
├── color_multiply.effect     # Shader for color effects
└── locale/                   # Translation files
\\\
## Building
### Prerequisites
- CMake 3.28 or higher
- OBS Studio source code and dependencies
- Visual Studio 2022 (Windows) or compatible compiler
### Windows Build
\\\ash
# Clone repository
git clone https://github.com/StreamUPTips/SUP_BB_Code.git
cd SUP_BB_Code
# Configure with CMake Presets
cmake --preset windows-x64
# Build
cmake --build build_x64 --config Release
\\\
### Linux Build
\\\ash
cmake --preset linux-x86_64
cmake --build build --config Release
\\\
### macOS Build
\\\ash
cmake --preset macos-universal
cmake --build build --config Release
\\\
## Installation
### Windows
1. Build the plugin (see Building section)
2. Copy sup-bbcode.dll to %AppData%/obs-studio/plugins/sup-bbcode/bin/64bit/
3. Copy shader files from data/ to %AppData%/obs-studio/plugins/sup-bbcode/data/
### Linux
1. Build the plugin
2. Copy sup-bbcode.so to ~/.config/obs-studio/plugins/sup-bbcode/bin/64bit/
3. Copy data files to ~/.config/obs-studio/plugins/sup-bbcode/data/
### macOS
1. Build the plugin
2. Copy sup-bbcode.so to ~/Library/Application Support/obs-studio/plugins/sup-bbcode/bin/
3. Copy data files to ~/Library/Application Support/obs-studio/plugins/sup-bbcode/data/
## Usage
1. Add a Text Source to your scene
2. In the source properties, enable BBCode formatting
3. Enter BBCode in the text field:
   - \[b]Bold Text[/b]\
   - \[i]Italic Text[/i]\
   - \[u]Underline[/u]\
   - \[c=FF0000]Red Text[/c]\
4. Click "BBCode Help / Tag Reference" to see all available tags
## Supported BBCode Tags
### Basic Formatting
- \[b]Bold[/b]\
- \[i]Italic[/i]\
- \[u]Underline[/u]\
- \[s]Strikethrough[/s]\
### Color & Style
- \[c=RRGGBB]Colored Text[/c]\
- \[bg=RRGGBB]Background Color[/bg]\
- \[size=20]Larger Text[/size]\
### Layout
- \[left]Left Aligned[/left]\
- \[center]Centered[/center]\
- \[right]Right Aligned[/right]\
### Advanced
- \[shadow=2,RRGGBB]Shadow Effect[/shadow]\
- \[glow=3,RRGGBB]Glow Effect[/glow]\
- \[animate=shake]Animated Text[/animate]\
See the help window in the plugin for a complete reference.
## Configuration
### Settings
- **Enable BBCode**: Toggle BBCode processing on/off
- **Refresh On Active**: Automatically refresh when source becomes active
- **Default Font**: Set default font for all text
- **Default Size**: Set default text size
### Macros
Save frequently used BBCode combinations as macros for quick insertion:
1. Click "Edit Macros" button
2. Create named macros with their BBCode tags
3. Use macros in text with \{macro_name}\
## Development
### Dependencies
- libobs (OBS Studio core library)
- obs-frontend-api (for UI features)
- Windows API (for help window on Windows)
### Code Style
- C++20 standard
- Follow OBS project conventions
- Use provided CMake helpers
### Build with Debug Symbols
\\\ash
cmake --preset windows-x64
cmake --build build_x64 --config Debug
\\\
## Contributing
Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request
## License
[Specify your license - MIT, GPL, etc.]
## Support
- �� [Documentation](https://docs.streamup.tips)
- 💬 [Discord Community](https://discord.streamup.tips)
- 🐛 [Report Issues](https://github.com/StreamUPTips/SUP_BB_Code/issues)
- 🌐 [Website](https://streamup.tips)
## Version History
### 0.1.0 (Current)
- Initial public release
- Core BBCode parsing and rendering
- Macro system
- Text effects and styling
- Help reference window
---
Made with ❤️ by StreamUP Tips
