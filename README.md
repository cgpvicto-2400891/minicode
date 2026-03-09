# MiniCode 🖥️

A lightweight, modern code editor for Linux — inspired by Visual Studio Code.

Built with **C++20**, **Qt6/Qt5**, and an integrated **LSP client** for clangd.

---

## ✨ Features

| Feature | Details |
|---|---|
| **Multi-tab editing** | Open multiple files with tabbed interface |
| **Syntax highlighting** | C++, Python, JavaScript/TypeScript, JSON, Bash… |
| **Error detection** | Local parser + LSP (clangd) diagnostics with red underlines |
| **Line numbers** | Highlighted current line, styled gutter |
| **Minimap** | VS Code–style code overview in the right margin |
| **Integrated terminal** | Run `gcc`, `g++`, `python`, `node` directly |
| **File explorer sidebar** | Browse your project tree |
| **LSP / clangd** | Autocomplete, go-to-definition, hover docs |
| **Dark theme** | VS Code Dark+ colour palette |
| **Auto-indent** | Smart newline indentation |
| **Bracket auto-close** | `{`, `(`, `[`, `"` auto-paired |
| **Keyboard shortcuts** | Full set (see below) |

---

## 🏗️ Architecture

```
MiniCode/
├── CMakeLists.txt
├── src/
│   ├── main.cpp               ← Entry point
│   ├── editor/
│   │   ├── Editor.cpp/hpp     ← Core editing widget (QPlainTextEdit)
│   ├── ui/
│   │   ├── MainWindow.cpp/hpp ← Application window, menus, tabs, terminal
│   ├── syntax/
│   │   ├── SyntaxHighlighter.cpp/hpp  ← Qt QSyntaxHighlighter + theme
│   ├── parser/
│   │   ├── CodeParser.cpp/hpp ← Rule-based parser + tokeniser
│   ├── lsp/
│   │   ├── LSPClient.cpp/hpp  ← JSON-RPC LSP client (stdio/clangd)
│   └── utils/
│       ├── FileManager.cpp/hpp ← File I/O, language detection
```

### Key design decisions

- **RAII everywhere** — smart pointers (`unique_ptr`) for owned resources
- **Signal/slot** — Qt signals connect LSP responses to the editor
- **Debounced diagnostics** — 800 ms after last keystroke, avoids thrashing
- **Incremental highlighting** — Qt calls `highlightBlock` per-block only
- **Non-owning LSP pointer** — `Editor` holds a raw `LSPClient*` (lifetime managed by `MainWindow`)

---

## 📦 Dependencies

| Dependency | Purpose |
|---|---|
| Qt 6 (or Qt 5.15+) | UI framework |
| CMake ≥ 3.20 | Build system |
| clangd (optional) | LSP server for C++ |
| GCC / Clang (C++20) | Compiler |

---

## 🔧 Installation

### Ubuntu / Debian

```bash
# Install Qt6 development packages
sudo apt update
sudo apt install -y \
    cmake build-essential \
    qt6-base-dev qt6-base-private-dev \
    libqt6widgets6 libqt6network6

# Optional: install clangd for full LSP support
sudo apt install -y clangd

# If Qt6 is unavailable, install Qt5:
# sudo apt install -y qtbase5-dev qtbase5-private-dev
```

### Build

```bash
git clone <repo-url> MiniCode
cd MiniCode
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Run

```bash
# Open the editor
./MiniCode

# Open specific files
./MiniCode main.cpp README.md

# Open a directory (use File → Open Folder from the UI)
```

---


### Run n'importe ou

```bash
# creation lien symbolique
 sudo ln -s /home/username/Downloads/MiniCode /usr/local/bin/minicode
```

---

## ⌨️ Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+N` | New file |
| `Ctrl+O` | Open file |
| `Ctrl+S` | Save |
| `Ctrl+Shift+S` | Save as |
| `Ctrl+W` | Close tab |
| `Ctrl+F` | Find in file |
| `Ctrl+/` | Toggle line comment |
| `Ctrl+Space` | Trigger LSP completion |
| `Ctrl+B` | Toggle sidebar |
| Ctrl+\` | Toggle terminal |
| `Ctrl+Shift+B` | Build (make) |
| `F5` | Run (./MiniCode) |
| `Ctrl++` | Zoom in |
| `Ctrl+-` | Zoom out |
| `Ctrl+scroll` | Font zoom in editor |
| `Tab` | Insert 4 spaces |
| `Enter` | Smart auto-indent |

---

## 🐛 Error Detection

### Local parser (always active)

The built-in `CodeParser` detects:

- **C++**: missing semicolons, unmatched `{}()[]`, unclosed `/*` comments
- **Python**: mixed tab/space indentation, missing `:` on compound statements
- **JavaScript**: unclosed template literals

Errors appear as **red wavy underlines**. Hover over the underlined text to see the error message in a tooltip.

### LSP diagnostics (requires clangd)

When `clangd` is running, real compiler diagnostics replace the local ones:

- Type errors
- Undeclared identifiers
- Missing headers
- clang-tidy warnings

The status bar shows `LSP: ● clangd` when the server is active.

---

## 🎨 Colour Theme

Based on VS Code **Dark+**:

| Token | Colour |
|---|---|
| Keywords | `#569cd6` (blue) |
| Types / std:: | `#4ec9b0` (teal) |
| Strings | `#ce9178` (orange) |
| Numbers | `#b5cea8` (green) |
| Comments | `#6a9955` (olive, italic) |
| Preprocessor | `#9b9b9b` (grey) |
| Error underline | `#ff0000` (red wavy) |
| Warning underline | `#ffcc00` (yellow wavy) |

---

## 🔭 LSP Integration

MiniCode connects to `clangd` using the **Language Server Protocol** (LSP 3.17) over stdio:

1. On startup, `MainWindow` calls `LSPClient::start("clangd", ...)`
2. `LSPClient` sends `initialize` + `initialized`
3. On every `textDocument/didChange`, clangd analyses the file
4. `publishDiagnostics` notifications are received and forwarded to `SyntaxHighlighter`
5. `Ctrl+Space` triggers `textDocument/completion`

---

## 📝 Usage Example

```bash
# Open a C++ file
./MiniCode hello.cpp

# In the editor:
# - Type code → syntax highlighting applies immediately
# - Save (Ctrl+S) → clangd analyses the file
# - Hover over a red underline → see error tooltip
# - Ctrl+Space → trigger autocomplete

# In the integrated terminal (Ctrl+`):
$ g++ -std=c++20 -o hello hello.cpp
$ ./hello
Hello, World!
```

---

## 🔮 Roadmap / Bonus Features

- [ ] Tree-sitter native integration (replace regex tokeniser)
- [ ] Git diff gutter (added/modified/deleted lines)
- [ ] Multiple cursors
- [ ] Split editor (side by side)
- [ ] Extension / plugin system
- [ ] Settings UI (font, theme, tab size)
- [ ] File search (Ctrl+P quick open)
- [ ] Bracket matching highlight
- [ ] Code folding

---

## 📄 License

MIT License — see LICENSE file.
