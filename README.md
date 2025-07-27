# Windows Grid Overlay

A simple, lightweight, and resizable grid overlay for Windows, ideal for gaming, design work, or aligning on-screen elements. The overlay is fully click-through and can be resized and moved on the fly with a global hotkey.
![screenshot](screenshot.png)
---

## Features

- **Custom Grid:** A clean 10x6 grid overlay.
- **Toggle Mode:** Switch between a locked, click-through overlay and an interactive resize mode.
- **Global Hotkey:** Use **Ctrl+Alt+G** to enter/exit resize mode at any time.
- **Persistent Position:** The application remembers its last size and position.
- **Lightweight:** Minimal resource usage and a single executable file.
- **System Tray Access:** Right-click the tray icon to exit or toggle resize mode.

## How to Use

1.  Run `grid_overlay.exe`.
2.  The grid will appear on your screen. In this default mode, all mouse clicks "pass through" the grid to the windows underneath.
3.  Press **Ctrl+Alt+G** to enter **Resize Mode**.
4.  A border and title bar will appear. The window is now solid and can be moved and resized.
5.  When you are done, press **Ctrl+Alt+G** or **ESC** to lock the grid. The borders will disappear, and it will become click-through again.
6.  To close the application, right-click its icon in the system tray and select **Exit**.

## How to Compile

To compile this project from source, you will need the MinGW-w64 toolchain with `g++` and `windres`.

1.  **Compile the resources:**
    ```bash
    windres resources.rc -o resources.o
    ```

2.  **Compile the application and link the resources:**
    ```bash
    g++ grid_overlay.cpp resources.o -o grid_overlay.exe -std=c++17 -static -static-libgcc -static-libstdc++ -mwindows -municode -lcomctl32 -lgdi32 -lshell32
    ```