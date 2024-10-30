# Yet Another Elden Ring Mod Loader

## Introduction
This is a mod loader for Elden Ring. It is inspred by [ModEngine](https://github.com/soulsmods/ModEngine2), and is designed to be simple and easy to use.

## Why Another Mod Loader?
- [ModEngine](https://github.com/soulsmods/ModEngine2) is great, but it is not maintained anymore.
- [me3](https://github.com/garyttierney/me3) is in development, but not so active.
- I just need a simple codebase which generates minimal overhead (less than 100KB at present).
- I may add new features eventually.

## Features
- Load dlls on game start
- Load mods like ModEngine

## Installation
1. Download the latest release.
2. You can choose either:
   1. Start the loader individually (recommended)
      1. Extract the zip file to ANY folder you like.
      2. Modify `YAERModLoader.ini` to fit your needs.
      3. Run `YAERModLoader.exe` to start the game.
      4. This is recommended because you can start the loader only when you need it and it won't pollute your game folder.
   2. Load the loader with game automatically
      1. Extract `YAERModLoader.dll` and `YAERModLoader.ini` to the game folder.
      2. Rename `YAERModLoader.dll` to either `dxgi.dll`, `dinput8.dll` or `winhttp.dll`.
      3. Modify `YAERModLoader.ini` to fit your needs.
      4. Run the game w/o EAC.

## Credits
- [ModEngine](https://github.com/soulsmods/ModEngine2): The original mod loader for Souls games.
- [Detours](https://github.com/microsoft/Detours): The library used to hook functions. I modified it not to use C++ features at all.
- [uthash](https://github.com/troydhanson/uthash): The library used to handle hash tables.
- [inih](https://github.com/benhoyt/inih): The library used to parse ini files.
- [wingetopt](https://github.com/alex85k/wingetopt): The library used to parse command line arguments.
