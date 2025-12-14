# Opus3D Test Adapter

A VS Code Test Explorer adapter for the Opus3D engine using Meson introspection.

## Features

- Discovers tests using `Opus3D-Test --list`
- Runs tests via `--run Suite.Test`
- Debugs tests using VS Code's C++ debugger
- Automatically finds the correct build config through Meson introspection
- Minimal configuration required

## Development

Open this folder in VS Code and press **F5** to launch an Extension Development Host.

Build using:

- npm install
- npm run build

Package the extension:

- vsce package

Install into your main VS Code instance:

- TODO