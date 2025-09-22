# General

- Make it easier to start a new project
- Create a single luna.c file that can be included in unity build setup
- Create luna.config were each project can set engine config
  - Resolution
  - Color palette
  - FPS
- Allow to set up FPS at runtime
- Allow to set up resolution at runtime
- Make sure there is no UB so I can use sanitazers for UB
  - Fix hashtable UB
  - Fix VLA UB

# Base

- Container utilities arr map etc double check

# Assets

- Generate single .pak or .resource or whatever file so that distribution is easier.
  - https://phoboslab.org/log/2024/09/qop
- Compress resources so that the download size on playdate takes less space
- Make more generic solution to stream assets, load and unload etc.
- Allow some asset types to not be included in some projects (BTrees don't need to be on every project)
- Multithread asset-gen

# Input

- Game pad support
- Better touch support

# Build

- Create lib that can be statically linked
- Hot relading
  - Everything is ready for doing it just need to spend a few days actually doing it
- Generate platform files if noot provided
- generic app manifest kind of file that sets the name of the app version etc
  - platoform files:
    - pdxinfo
    - macos.plist
    - www.manifest

# GFX

- CRT effect and more postprocessing effects
- Support for CPU rasterizer with RGBA support
- Allow to set up pixel perfect at runtime
- Fix Circle rendering, currently only odd sized circles can be drawn
  - Check what PICO-8 does and try to replicate it

# Audio

- Fix desktop audio stuttering
- Support [QOA audio format](https://qoaformat.org/)

# WWW

- Generate shell html file with touch controls similar to PICO-8

# Contrib

- Create new folder with libs that are commonly re-used on projects but are inteded to be copied and pasted and modified on each project
  - globals with global refs to assets.
  - global gfx state for easier drawing
  - global sfx state for easier playing sounds

# UI

- Make a ui system that's easy to build tools and editors with

# Serialization

- Autogenerate serialization functions and data types

# Platforms

- Android
- iOS

# Desktop

- Multiwindow

# Tracing

- Improve spall on device (playdate)
- Auto tracing
