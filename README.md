# Unlock the Door: Culinary Trials

A native C++17 desktop game with a graphical interface, built with raylib.

## Features

- Three animated doors; exactly one is the winning door each round.
- Nine culinary questions across three difficulty tiers, with shuffled answer choices.
- Rank up from Home Cook to Line Cook to Head Chef after consecutive wins.
- A score and kitchen-streak combo: consecutive correct answers earn bonus points.
- Unlocked rank, personal best score, and best streak saved locally in `unlock_records.txt`.
- Mouse controls and keyboard shortcuts.
- Confetti celebration, win/loss reveal, replay, and resizable window.

## Requirements

- Windows 10 or newer
- Visual Studio 2022 with the **Desktop development with C++** workload
- CMake 3.24 or newer
- Internet connection on the first configure, so CMake can download raylib 5.5

## Build and run

Open a Visual Studio Developer PowerShell in this folder and run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\unlock_the_door.exe
```

CMake downloads and builds raylib automatically. No separate raylib installation is needed.

## Controls

- Click a door, or press `1`, `2`, or `3` to choose it.
- Click an answer, or press `1`, `2`, `3`, or `4` to answer the culinary question.
- Click **Play Again** to start a new round.
- Press `Esc` to return to the title screen.

The existing GitHub Pages game is still the browser-based HTML version. This project is the native C++ desktop version; hosting C++ in a browser would require a separate WebAssembly build.
