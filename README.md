# Wii 3D Engine

A lightweight 3D engine for the Nintendo Wii built on top of Wire3D.

This project began as an experiment to see how far modern-style 3D rendering and physics could be pushed on Wii hardware while keeping the codebase simple and easy to understand.

The engine currently powers a small physics sandbox featuring rigid body simulation, object spawning, and real-time interaction using the Wii Remote.

## Why?

The Nintendo Wii is a surprisingly capable system, but there are very few modern resources available for creating 3D applications from scratch.

This project exists to explore:

- Real-time 3D rendering on Wii
- Physics simulation
- Asset loading and custom file formats
- Engine architecture for constrained hardware
- Homebrew game development

## Features

- 3D rendering using Wire3D
- Physics simulation
- Wii Remote input
- Optimized for real hardware
- Open source
- Built entirely with C++

## Demo

The included physics demo allows you to:

- Spawn dynamic objects
- Interact with stacks of rigid bodies
- Enable slow motion effects
- Test engine performance on real Wii hardware

## Requirements

Install devkitPro:

https://devkitpro.org/wiki/Getting_Started

## Building

Clone the repository:

```bash
git clone https://github.com/DavidSkillman/wii-3d-engine.git
cd wii-3d-engine
```

Initialize the Wire3D submodule:

```bash
git submodule update --init --recursive
```

Build Wire3D:

```bash
cd wire3d/Wire/builds/gcc_wii
make -j8
```

Build the engine:

```bash
cd ../../../../builds/gcc_wii
make -j8
```

## Running

After compilation, the generated `.dol` file can be loaded on a Nintendo Wii using your preferred homebrew loader.

## Project Structure

```yaml
wii-3d-engine/
├── builds/
├── data/
├── src/
├── wire3d/
└── source/
```

## License

This project is licensed under the MIT License. See `LICENSE` for details.

---

Made with ❤ by David Skillman
