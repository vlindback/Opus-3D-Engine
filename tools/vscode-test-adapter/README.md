# Opus3D Test Adapter

A VS Code Test Explorer adapter for the Opus3D engine using Meson introspection.

## Features

- Discovers tests using `Opus3D-Test --list` and refreshes automatically when files in `build/` change.
- Runs tests via a filter `--run Category/Suite/Test` or the whole suite when no filter is provided.
- Debugs tests using VS Code's C++ debugger with a single selection.
- Parses failure output into editor diagnostics so you can jump to the failing line.
- Automatically finds the correct build config through Meson introspection, with an optional manual `opus3dTests.executablePath` override.

## Development

Open this folder in VS Code and press **F5** to launch an Extension Development Host.

Build using:

- npm install
- npm run build

Package the extension:

- vsce package

Install into your main VS Code instance:

- code --install-extension tools\vscode-test-adapter\opus3d-test-adapter-0.0.1.vsix
