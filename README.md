# Pokemmo Egg Grid Overlay

A lightweight, resizable grid overlay specifically designed to speed up the process of shiny hunting eggs in Pokemmo.

Stop losing track of which row you're on. Place a persistent marker over the "Breed" button. Shave seconds off every cycle and save hours on your next hunt.

![screenshot](https://i.gyazo.com/a808b21631b04827ae772c6bf7fec86b.png)
![screenshot](https://i.gyazo.com/ec26527051d5b654d700df06ace455e7.png)
![screenshot](https://i.gyazo.com/387f7765f6d306fbab26d32854cb09f0.png)
---

This tool overlays a simple, clean **10x6 grid** onto your screen that you can perfectly align with your Pokemmo PC box. It is for **Windows only**.

## Quickstart Guide

1.  Download the latest `grid_overlay.exe` from the [**Releases**](https://github.com/sdinukad/egg-grid-overlay/releases) page.
2.  Run the application. The grid will appear.
3.  Press **Ctrl+Alt+G** to enter **Interactive Mode**. The grid will get a border and become solid.
4.  Drag and resize the grid until it aligns perfectly with your PC boxes.
5.  While in Interactive Mode:
    *   **Left-click** to place a persistent red dot over a key location (like the "Breed" button).
    *   **Right-click** to remove the dot.
6.  Once aligned, press **Ctrl+Alt+G** or **ESC** to lock the grid. The borders will vanish, and the overlay will become click-through again.

## Features

- **Perfect Fit:** A 10x6 grid designed to align with your PC box.
- **Numbered Columns:** The top row is numbered 1-10 for instant column identification and help you keep track.
- **Custom Marker:** Place a persistent red dot to mark your breed button spot.
- **Toggle Interactive Mode:** A global hotkey (**Ctrl+Alt+G**) lets you adjust the grid's size, position, and marker on the fly.
- **Click-Through:** When locked, the overlay is completely invisible to your mouse, allowing you to play normally.
- **Persistent Memory:** The app saves its last position and marker location, so you only have to set it up once.
- **Lightweight:** A single, tiny executable with minimal resource usage. It just works.

## Compiling From Source

If you want to build the project yourself, you'll need the MinGW-w64 toolchain (`g++` and `windres`).

1.  **Compile the Windows resources:**
    ```bash
    windres resources.rc -o resources.o
    ```

2.  **Compile the C++ source and link everything:**
    ```bash
    g++ grid_overlay.cpp resources.o -o grid_overlay.exe -std=c++17 -static -static-libgcc -static-libstdc++ -mwindows -municode -lcomctl32 -lgdi32 -lshell32
    ```

## Credits
Got the idea from seeing it on the twitch stream of PaulusTFT - http://twitch.tv/paulustft

## Disclaimer

This is a third-party, fan-made tool and is not affiliated with the PokeMMO project or its developers. 
It is a simple screen overlay and does not interact with the game's memory, packets, or executable in any way. 
It complies with the fair play rules by acting as a simple visual aid, similar to putting a transparent sticker on your monitor.