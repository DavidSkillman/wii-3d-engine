# Wii 3D Engine
This project began as an experiment to explore how far modern-style 3D rendering, physics simulation, and custom asset pipelines could be pushed on Nintendo Wii hardware while remaining lightweight, approachable, and efficient.

The engine currently powers a physics sandbox featuring real-time rigid body simulation, object spawning, collision detection, and Wii Remote interaction. Alongside the engine, a dedicated asset pipeline allows models to be converted into a custom `.wmesh` format optimized specifically for the constraints of the Wii.

## Why?

The Nintendo Wii remains one of the most successful game consoles ever created, yet modern resources for low-level 3D game development on the platform are surprisingly limited.

This project exists as both a learning exercise and a practical game development framework, exploring topics such as:

* Real-time 3D rendering
* Physics simulation
* Asset pipelines and custom file formats
* Performance optimization on constrained hardware
* Homebrew game development
* Engine architecture and tooling

## Features

### Rendering

* 3D rendering powered by Wire3D
* Real-time camera movement
* Hardware-accelerated graphics
* Optimized for native Wii hardware

### Physics

* Rigid body simulation
* Collision detection
* Dynamic object spawning
* Interactive physics sandbox

### Asset Pipeline

* Custom `.wmesh` format
* Fast binary asset loading
* Reduced runtime processing requirements
* Integration with the [Wii Mesh exporter](https://github.com/DavidSkillman/wii-mesh)

## Demo

The included physics demo showcases the engine's rendering and simulation capabilities.

### Controls

| Button           | Action             |
| ---------------- | ------------------ |
| 1                | Spawn two spheres  |
| 2 (Hold)         | Enable slow motion |
| D-Pad            | Look               |
| A                | Move Forwards      |
| B                | Move Backwards     |
| +                | Ascend             |
| -                | Descend            |
| Home             | Quit               |

### Features Demonstrated

* Real-time physics simulation
* Object stacking and collisions
* Dynamic rigid body spawning
* Large numbers of active physics objects
* Real hardware performance testing

## Building

**Make sure you have [devkitPro](https://devkitpro.org/wiki/Getting_Started) installed.**

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

```text
wii-3d-engine/
├── builds/        # Build scripts and makefiles
├── data/          # Assets and resources
├── source/        # Engine source code
├── src/           # Third-party dependencies
└── wire3d/        # Wire3D submodule
```

## Related Projects

### Wii Mesh

The recommended asset pipeline for the engine.

Wii Mesh imports common 3D model formats through Assimp and converts them into the custom `.wmesh` format used by the engine. The exporter also performs optimization and automatic mesh chunking for compatibility with Wii hardware.

## License

This project is licensed under the MIT License. See `LICENSE` for details.

---

Made with ❤️ by David Skillman
