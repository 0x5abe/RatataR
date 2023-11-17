# RatataR

RatataR is a custom launcher for the PC version of Ratatouille.
It currently supports overlay.exe for the USA version of Ratatouille.

## List of features
- Custom resolution
- Windowed mode
- Borderless mode
- Developer console
- Popup menu
- Invert mouse on the Y-axis
- Custom Field Of View
- Remove framerate cap
- No CD patch
- Auto save toggle
- Speedrun mode

### Planned features
- Support for other versions of the game
- Overall rewrite of the tool using minhook, for more extensive modding
- Fps counter and limiter

## Usage

Download the latest version from the [Releases section](https://github.com/SabeMP/RatataR/releases).

<p>Unzip the files to your Ratatouille game folder, which is typically found at the following path: <br>C:\Program Files (x86)\THQ\Disney-Pixar\Ratatouille\Rat</p>

Run RatataR.exe to launch the game with RatataR.

By default, the game gets set to the resolution of your primary display. You can adjust these settings by editing the RatataRconfig.ini file with a text editor. The available options are:
  
- Window Mode:
  - Windowed: The game runs in a window, allowing for seamless multitasking.

  - Fullscreen: Uses the game's native fullscreen mode to occupie the entire screen. May cause compatibility issues with screen capture software. Please be aware that the game's display resolution is determined by the settings configured in the gamesetup.exe graphics settings.

  - Borderless: The game operates within a window but with no borders, providing the illusion of fullscreen while retaining the ability to easily multitask.

- Screen Resolution:
  - Set the width and height in pixels using the 'width' and 'height' parameters.

- Developer Console: [OFF / ON]
  - Show/hide the console window.

- Popup Menu: [OFF / ON]
  - Toggles the popup menu, accessible with a right-click.

- Invert Vertical Look: [OFF / ON]
  - Inverts the mouse Y-axis.

- Auto Save: [OFF / ON]
  - Toggles the games autosaving feature.

- Field Of View: [1-155]
  - Adjust the field of view.

- Remove FPS Cap: [OFF / ON]
  - Removes the 500 fps cap enforced by the game.

- Speedrun Mode: [OFF / ON]
  - Disables modifications that are not legal for speedrunning.

### No disc patch
1. If you have multiple discs, mount/insert the second game disc.
2. Open File Explorer.
3. Navigate to the "RAT2" disc.
4. Go to the "View" tab and select "Show" and then "Hidden Items."
5. Copy the "MUSIC" and "VIDEOS" folders into your Ratatouille game folder.
