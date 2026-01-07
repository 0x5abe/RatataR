# RatataR

A launcher designed to enhance the experience of the **Ratatouille** video game for PC.

> [!NOTE]
> Some game versions may not be supported. If you're having problems, feel free to open up an issue.

## Features
- Custom Resolution
- Windowed/Borderless Mode
- Developer Tools
- Graphical and Gameplay Improvements
- Frame Limiter
- No Disc Requirement

### Planned features
- Overall rewrite of the tool using MinHook for more extensive modding.

## Usage

Download the latest version from the [Releases section](https://github.com/SabeMP/RatataR/releases).

<p>Unzip the files to your Ratatouille game folder, which is typically found at the following path: <br>C:\Program Files (x86)\THQ\Disney-Pixar\Ratatouille\Rat</p>

Run **RatataR.exe** to launch the game with RatataR.

You can adjust settings by editing RatataRconfig.ini with a text editor.
> [!TIP]
> You may need to run your text editor with **administrator privileges** if editing the file while it's 
inside the game folder.

### Play without disc
If you want the ability to play without needing to have the disc inserted, you need to copy the game files from the disc into the game directory.
1. If you have multiple discs, mount/insert the second game disc.
2. Open File Explorer.
3. Navigate to the **RAT2** disc.
4. Go to the **View** tab and select **Show** and then **Hidden Items**.
5. Copy the **MUSIC** and **VIDEOS** folders into the root of the game folder.

# Credits
**RatataR** makes use of third-party libraries:
- **MinHook** — A Minimalistic API Hooking Library for x64/x86  
Copyright (C) 2009-2017 Tsuda Kageyu.

- **Hacker Disassembler Engine 32 C**  
Copyright (c) 2008-2009, Vyacheslav Patkov.

- **Discord rich-presence** by [Jay!](https://twhl.info/user/view/7970).

See [THIRD_PARTY_LICENSES.txt](THIRD_PARTY_LICENSES.txt) for details.

# DISCLAIMER
This application is a fan-made project and is in no way affiliated with or approved by **Disney Pixar**, **Asobo Studio**, **THQ**, or any other copyright holder.
No copyrighted game assets are distributed and a legally obtained copy of **Ratatouille** is required to use **RatataR**.
