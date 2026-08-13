# General

- [ ] Make it easier to start a new project
- [~] Allow to set up FPS at runtime
  - [ ] Review FPS specially on desktop macos is showing wierd things
  - [ ] Improve sys-font
- [ ] Allow to set up resolution at runtime

# Base

- [ ] Improve how log is setup in sys
- [ ] Container utilities arr map etc double check
- [ ] use uint_64_t and friends in base and core only use typedefs for app.
- [ ] remove the use of arr and friends define slices
- [ ] remove arr_push_pack
- [ ] remove cir buffer

# CLI

- [ ] use cmd_line.h

# Assets

- [ ] Generate single .pak or .resource or whatever file so that distribution is easier.
  - https://phoboslab.org/log/2024/09/qop
- [ ] Compress resources so that the download size on playdate takes less space
- [ ] Make more generic solution to stream assets, load and unload etc.
- [ ] Allow some asset types to not be included in some projects (BTrees don't need to be on every project)
- [ ] Multithread asset-gen

# Input

- [ ] Game pad support
- [ ] Better touch support

# Build

- [ ] Cross build from Linux to mac
- [ ] Create lib that can be statically linked
- [ ] Hot relading
  - Everything is ready for doing it just need to spend a few days actually doing it
- [ ] Generate platform files if noot provided
- [ ] generic app manifest kind of file that sets the name of the app version etc
  - platoform files:
    - pdxinfo
    - macos.plist
    - www.manifest

# GFX

- [ ] CRT effect and more postprocessing effects
- [ ] Support for CPU rasterizer with RGBA support
- [ ] Allow to set up pixel perfect at runtime
- [ ] Fix Circle rendering, currently only odd sized circles can be drawn
  - Check what PICO-8 does and try to replicate it

# Audio

- [ ] Support [QOA audio format](https://qoaformat.org/)

# WWW

- [ ] Generate shell html file with touch controls similar to PICO-8

# Contrib

- [ ] Create new folder with libs that are commonly re-used on projects but are inteded to be copied and pasted and modified on each project
  - [ ] globals with global refs to assets.
  - [ ] global gfx state for easier drawing
  - [ ] global sfx state for easier playing sounds

# UI

- [ ] Make a ui system that's easy to build tools and editors with

# Serialization

- [ ] Autogenerate serialization functions and data types

# Platforms

- [ ] Android
- [ ] iOS

# Desktop

- [ ] Multiwindow

# Tracing

- [ ] Add support for prof history like iProf
- [ ] Graphs

# Frame pacing

- [ ] Glaiel delta snapping
      Round `time_delta` to the nearest multiple of `dt_us` when within ~2 ms.
      Only worth it for leftover `M01`–`M03` on cheap screens (occasional double-step from the panel
      not being exactly 20.000 ms). It will not fix slow games going over the 20ms budget
- [ ] Gaffer render interpolation (`alpha = acc_us / dt_us`)
      Draws bodies between previous and current transforms so a leftover miss does not double-step
      on screen. Needs previous-and-current poses per body.
      Desktop judder (60/120/144 vs 50) would also improve.
- [ ] Pause / resume through the system menu
      Ball rolling, menu ~30 s then ~5 min, then sleep past ~4295 s (old float-to-u32 UB).
      Ball must not jump; challenge-timer and ball-saver must not drift.

# Physics

- [ ] Gravity inside the substep loop, real `dt` on `body_integrate`
      `vel` is pixels-per-substep, not pixels-per-second, and gravity only hits substep 1.
      Correct, and retunes every table: ball speed scales with `UPS x steps`, effective g with
      `dt / steps`.
- [ ] Real per-substep displacement cap `max_translation` / `max_rotation` are inert (`max_linear_speed` is ~1600 vs
      velocities under 10). The 4 substeps are the only tunneling guard.
