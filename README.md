# Tools for Elden Ring
Written by [Lua](https://www.lua.org/)

#### Notes
* Please start `Elden Ring` without EAC, there are two methods:
  1. (Suggested) Create a `steam_appid.txt` in the same folder of `eldenring.exe`, put `1245620` in the txt file, then start game with `eldenring.exe`.
  2. Backup and delete `start_protected_game.exe`, rename `eldenring.exe` to `start_protected_game.exe`

## Live-Streaming Tool
#### Usage
* Start
  + Run `livestreamingtool.bat`, it will find running game process automatically.
    - When game is started, the tool will try to output to text files from reading game memory blocks
    - When game is closed, output text files will be cleaned up
  + By default, the tool writes to `info.txt`, `items.txt` and `bosses.txt` in `output` folder, you can read them in your live-streaming tools (Set text Encoding to `UTF-8` for some tools)
    - `info.txt`: Game and character information, including game time, NG+ count, death count, runes, character levels and attributes.
    - `items.txt`:
      - Rread bag and box to calculate progress of item collections, weapons and armors are supported currently.
      - Display current equipped items.
    - `bosses.txt`: show progress of boss slains, you can switch the output in 2 modes:
      - 165 bosses (Display total progression and current region progression).
      - 15 rememberance bosses.
* Customization
  + Edit `_config_.lua` in folder `livestreamingtool`, to change some options
  + Modify `info.lua`, `items.lua` and `bosses.lua` in folder `livestreamingtool` to change output content. You may need some knowledge about Lua language.
