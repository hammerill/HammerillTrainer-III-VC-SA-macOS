# Grinch Trainer
## Introduction

A trainer Grand Theft Auto III, VC & SA enabling a much more easier & amusing gameplay with cheats. This is **NOT** intended for use in multiplayer, I won't be responsible for any form of bans. Use it at your own risk!

Press <kbd>Left Ctrl</kbd> + <kbd>T</kbd> (on keyboard) or <kbd>GamePadFaceLeft</kbd> + <kbd>GamePadFaceDown</kbd> (on consoles - check keymappings section) to open the trainer.

<details>
<summary>Gamepad Key Mappings</summary>

1. **GamepadStart**
   - Xbox: Menu
   - Switch: + (Plus)
   - PS: Options

2. **GamepadBack**
   - Xbox: View
   - Switch: - (Minus)
   - PS: Share

3. **GamepadFaceLeft**
   - Xbox: X
   - Switch: Y
   - PS: Square

4. **GamepadFaceRight**
   - Xbox: B
   - Switch: A
   - PS: Circle

5. **GamepadFaceUp**
   - Xbox: Y
   - Switch: X
   - PS: Triangle

6. **GamepadFaceDown**
   - Xbox: A
   - Switch: B
   - PS: Cross

7. **GamepadDpadLeft**
   - Xbox: D-pad Left
   - Switch: D-pad Left
   - PS: D-pad Left

8. **GamepadDpadRight**
   - Xbox: D-pad Right
   - Switch: D-pad Right
   - PS: D-pad Right

9. **GamepadDpadUp**
   - Xbox: D-pad Up
   - Switch: D-pad Up
   - PS: D-pad Up

10. **GamepadDpadDown**
    - Xbox: D-pad Down
    - Switch: D-pad Down
    - PS: D-pad Down

11. **GamepadL1**
    - Xbox: L Bumper
    - Switch: L
    - PS: L1

12. **GamepadR1**
    - Xbox: R Bumper
    - Switch: R
    - PS: R1

13. **GamepadL2**
    - Xbox: L Trigger
    - Switch: ZL
    - PS: L2

14. **GamepadR2**
    - Xbox: R Trigger
    - Switch: ZR
    - PS: R2

15. **GamepadL3**
    - Xbox: L Stick (click)
    - Switch: L3
    - PS: L3

16. **GamepadR3**
    - Xbox: R Stick (click)
    - Switch: R3
    - PS: R3

17. **GamepadLStickLeft**
    - Xbox: Left Analog Stick Left
    - Switch: Left Analog Stick Left
    - PS: [Analog] Move Window (in Windowing mode)

18. **GamepadLStickRight**
    - Xbox: Left Analog Stick Right
    - Switch: Left Analog Stick Right
    - PS: [Analog] Move Window (in Windowing mode)

19. **GamepadLStickUp**
    - Xbox: Left Analog Stick Up
    - Switch: Left Analog Stick Up
    - PS: [Analog] Move Window (in Windowing mode)

20. **GamepadLStickDown**
    - Xbox: Left Analog Stick Down
    - Switch: Left Analog Stick Down
    - PS: [Analog] Move Window (in Windowing mode)

21. **GamepadRStickLeft**
    - Xbox: Right Analog Stick Left
    - Switch: Right Analog Stick Left
    - PS: [Analog]

22. **GamepadRStickRight**
    - Xbox: Right Analog Stick Right
    - Switch: Right Analog Stick Right
    - PS: [Analog]

23. **GamepadRStickUp**
    - Xbox: Right Analog Stick Up
    - Switch: Right Analog Stick Up
    - PS: [Analog]

24. **GamepadRStickDown**
    - Xbox: Right Analog Stick Down
    - Switch: Right Analog Stick Down
    - PS: [Analog]

</details>

Get stable binaries from [here](https://github.com/user-grinch/GrinchTrainer-III-VC-SA/releases). Beta binaries are available at the discord server.

[![Discord Banner 2](https://discordapp.com/api/guilds/689515979847237649/widget.png?style=banner2)](https://discord.gg/AduJVdyqCD)
[![375333774-f8adb118-0c50-4ce7-91c0-981cd366e6d6](https://github.com/user-attachments/assets/e6aa195c-e93a-41a9-8b14-2788bed2d7d1)](https://www.patreon.com/grinch_)

## Installation
Only these versions of the game is supported. I highly recommand installing the menu on a fresh, non-modded game first.
- GTA SA v1.0 US Hoodlum 
- GTA VC v1.0 EN
- GTA III v1.0 EN

If you have those mentioned versions installed,
1. Install [DirectX9](https://www.microsoft.com/en-us/download/details.aspx?id=35), [Visual C++ Redistributable 2022 x86](https://aka.ms/vs/17/release/vc_redist.x86.exe) & [Ultimate asi loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases).
2. Download [**GrinchTrainerSA/VC/III.7z**](https://github.com/user-grinch/GrinchTrainer-III-VC-SA/releases) and extract everything in game root directory (NOT MODLOADER). The **GrinchTrainer** folder & **.asi** file must be placed in the same place.
3. Install [SilentPatch](https://gtaforums.com/topic/669045-silentpatch/) (For GTA III don't install the ddraw.dll component)
4. Install [D3D8to9 wrapper](https://github.com/crosire/d3d8to9/releases) (For III & VC) 

Check out the [Compatibility List](https://docs.google.com/document/d/1BrDlFQ_w25AOx8mMJe0PQkuOpJL4LezXMS6TlUpS5XU/edit?usp=sharing)

## macOS (GTA Vice City) — Hammerill Trainer
Hammerill Trainer is a standalone trainer for the macOS build of GTA Vice City (the unofficial **reVC** recompilation for Apple Silicon, `GTA Vice City.app`). It's a single plugin file. You **don't** need an ASI loader, DirectX, d3d8to9, SilentPatch or any other mod; the data files from `resource/GrinchTrainerVC` are built into the plugin. It's based on Grinch Trainer by Grinch_ and uses its data files and font.

**Requirements**
- A Mac with Apple Silicon (M1 or later), and GTA Vice City for macOS installed, normally at `/Applications/GTA Vice City.app`. When the app is set up correctly, `Contents/MacOS` contains a `reVC` launcher script and a `macosx-arm64-librw_gl3_glfw-oal` executable.
- Xcode Command Line Tools (`xcode-select --install`) and CMake (`brew install cmake`) to build the plugin.

**Build**

Run these commands in the repository folder:
```sh
cmake -S macos -B build
cmake --build build
```
This downloads Dear ImGui and the GLFW headers (pinned versions with checked hashes) and produces `build/HammerillTrainerVC.dylib`. If the game is somewhere other than `/Applications`, add `-DGTAVC_APP="/path/to/GTA Vice City.app"` to the first command.

**Install**
```sh
./macos/install.sh
```
The script does two things:
1. It copies `HammerillTrainerVC.dylib` into `GTA Vice City.app/Contents/Frameworks/`.
2. It edits the game's launcher script `Contents/MacOS/reVC` so the game loads the plugin at startup, and saves the original script as `reVC.hammerilltrainer-backup`.

If you installed the game somewhere else, pass both paths: `./macos/install.sh build/HammerillTrainerVC.dylib "/path/to/GTA Vice City.app"`. After rebuilding, run the script again to update the plugin.

Then start the game as usual (Finder, Launchpad or Dock). Load or start a game and press <kbd>Left Ctrl</kbd> + <kbd>T</kbd> to open or close the trainer. On a gamepad, press X + A (Xbox) or Square + Cross (PlayStation). <kbd>Esc</kbd> also closes it. While the trainer is open, the game ignores your mouse, keyboard and gamepad.

**Uninstall**
```sh
./macos/uninstall.sh
```
This restores the original launcher and deletes the plugin. Your saves are not affected.

**Features:** god mode, never wanted, infinite ammo and sprint, health, armour, money and wanted level, give any weapon, spawn any vehicle (optionally with you inside), repair and flip your vehicle, vehicle god mode, teleport (map marker, coordinates or the location list), time, weather, game speed, ped and traffic density, HUD toggle, and every built-in cheat.

**Customising lists:** to replace a built-in list, put your own `vehicles.toml`, `weapons.toml` or `locations.toml` (same format as `resource/GrinchTrainerVC/data`) in a `HammerillTrainerVC` folder inside `GTA Vice City.app/Contents/Resources/`.

**Troubleshooting:** the plugin writes `HammerillTrainerVC.log` to `GTA Vice City.app/Contents/Resources/`. The plugin was built for one specific reVC build, so it checks the game's memory layout at startup. With a different build, the trainer window explains that the game version isn't supported and stays disabled, so it can't corrupt the game. If the game doesn't start, run `./macos/uninstall.sh` and report the problem with the log attached.

<details>
<summary>How it works</summary>

The game is ad-hoc signed without the hardened runtime. As a result, `DYLD_INSERT_LIBRARIES`, set by the launcher script, can load the plugin. The plugin uses dyld interposing on the GLFW functions the game imports. It draws the menu (Dear ImGui) in `glfwSwapBuffers` and filters input through the GLFW callbacks and polling functions. It never patches game code. It finds game functions by their exported symbols (reVC ships with them). It reads struct field offsets from the game's own instructions, and `macos/src/game.cpp` re-checks each offset at startup.
</details>

## Images
<details>
  <summary>Show Images</summary>
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(1).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(2).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(3).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(4).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(5).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(6).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(7).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(8).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(9).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(10).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(11).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(12).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(13).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(14).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(15).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(16).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(17).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(18).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(19).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(20).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(21).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(22).png">
  <img src="https://raw.githubusercontent.com/user-grinch/GrinchTrainer-III-VC-SA/master/images/screen%20(23).png">
</details>


## Things to do on crash
1. Delete the `GrinchTrainerXX.toml` config file and try again.
2. If it's happening after installing a recent mod try removing it.
3. Try on a fresh installation of the game.
4. If still doesn't fix your issue, go ahead and report it.

### Crash reports/ Bug reports
Install [Crash info](https://www.mixmods.com.br/2021/06/crashinfo.html) & reproduce your crash. When reporting the crash upload the crashinfo logfile with it. Explain how to reproduce the crash in details. Also include,
- Game version
- Menu version
- List of mods installed

You can either open a issue here or directly contact me on discord (Recommended).
<!-- 
## Images
<details>
  <summary>Show Images</summary>
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/1.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/2.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/3.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/4.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/5.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/6.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/7.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/8.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/9.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/10.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/11.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/12.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/13.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/14.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/15.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/16.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/17.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/18.png">
  <img src="https://raw.githubusercontent.com/user-grinch/Cheat-Menu/master/images/19.png">
</details> -->

## Submit Translations
*How to translate trainer to another language*

The translations are loaded from `GrinchTrainerXX/locale/*.toml` files. Make a copy of the `English.toml` file and translate the texts there to your language. Rename the file to `your_language.toml` and make a pull request. 

If you're language isn't supported ( Appears `???` in the menu. Check [here](https://github.com/user-grinch/GrinchTrainer-III-VC-SA/tree/master/tools/in.txt) for supported languages), open a [issue](https://github.com/user-grinch/GrinchTrainer-III-VC-SA/issues) asking for support.

All credits are given in Menu/ About tab.
