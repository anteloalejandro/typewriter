# Typewritter

A bothersome time waster for the fun of it.

Made with raylib and raygui.

## Build

### Install dependencies

You need to install some required libraries; ALSA for audio, Mesa for OpenGL accelerated graphics and X11 for windowing system.

**Ubuntu**

```bash
sudo apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
```

**Fedora**

```bash
sudo dnf install alsa-lib-devel mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel libXcursor-devel libXinerama-devel libatomic
```

**Arch Linux***

```bash
sudo pacman -S alsa-lib mesa libx11 libxrandr libxi libxcursor libxinerama
```

### Run build script

```bash
./build.sh
```

### Run executable

```bash
./bin/main
```

## TODO

- [ ] **Typing like you'd do in a typewritter**
  - [x] Tabs, space and backspace only move the cursor
  - [x] Enter jumps to a new line, but it does not act like a CR
  - [x] Carriage Return key
  - [ ] Key to delete text as if the typewritter had a type with corrector fluid.
  Deleting won't be visually perfect and may take multiple keystrokes
  - [ ] Allow writing multiple characters in the same area, one over another
  - [x] Move the mouse to drag the page arround, along with the cursor position
  - [ ] Notify the user when only 10 characters are left to hit the margin
  - [ ] Add a ruler to set margins
  - [x] Move the page around as you write
- [ ] **Style**
  - [ ] Satisfying sound effects and animations
  - [ ] Multiple "typewritters" (different fonts and UI)
- [ ] **Interactive things, but *more obnoxious***
  - [ ] *Monkeytype* style typing test
  - [ ] *Typing of the Dead* style minigame
- [ ] **Extras**
  - [ ] Options to enable features not found in a typewritter or disable some that are (see above)
  - [ ] Unicode support
  - [ ] Cross-platform
