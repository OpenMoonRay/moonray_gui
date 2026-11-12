# moonray_gui_v2

moonray_gui_v2 is an imgui-based application (glfw/opengl3) that replaces the existing QT
application. It currently builds on both linux and mac.

## Getting Started

After cloning this project, you must run `git submodule update --init --recursive` in order to initialize and update 
the imgui submodule. 

To run, simply use `moonray_gui_v2` instead of `moonray_gui`.

## Usage
While future UI improvements are forthcoming, moonray_gui_v2 currently relies on hotkeys for user 
interaction. It mostly uses the same hotkeys as moonray_gui v1, but a few have been 
changed. Use hotkey `H` to print all the hotkeys to the console.
Here are the hotkey mappings (the changed hotkeys have stars next to them):

### Camera Hotkeys
| Action | Hotkey | Notes |
| ------ | ------ | ----- |
| Toggle Camera Type | `O` | |
| Forward | `W` | |
| Backward | `S` | |
| Left | `A` | |
| Right | `D` | |
| Up | `SPACE` | |
| Down | `C` | |
| Slow Down | `Q` | |
| Speed Up | `E` | |
| Reset | `R` | |
| Recenter | `F` | |
| Frame Scene | `SHIFT` + `F` | new in v2 |
| Print Matrices* | `M` | Previously `T` |
| Set Up Vector | `U` | |
| Orbit* | `ALT`+`LMB` | FreeCam previously used `LMB` only |
| Dolly | `ALT`+`RMB` | |
| Pan | `ALT`+`MMB` | |
| Roll* | `CTRL`+`LMB` | Previously `ALT`+`LMB`+`RMB` |

### Denoising Hotkeys
| Action | Hotkey |
| ------ | ------ |
| Toggle On/Off | `N` |
| Toggle Mode | `SHIFT`+`N` |
| Select Buffers | `B` |

### Channel Hotkeys
| Action | Hotkey | Notes |
| ------ | ------ | ----- |
| RGB | `\`` | |
| Red | `1` | |
| Green | `2` | |
| Blue | `3` | |
| Alpha | `4` | |
| Luminance | `5` | |
| RGB Normalized* | `6` | Previously `7` |
| Num Samples* | `7` | Previously `8` |

### Color Management Hotkeys
| Action | Hotkey |
| ------ | ------ |
| Exposure Increase | `UP` |
| Exposure Decrease | `DOWN` |
| Exposure Adjust | `X`+`LMB` |
| Exposure Reset | `SHIFT`+`X` |
| Gamma Adjust | `Y`+`LMB` |
| Gamma Reset | `SHIFT`+`Y` |

### Fast Progressive Mode Hotkeys
| Action | Hotkey |
| ------ | ------ |
| Toggle On/Off | `L` |
| Next Mode | `ALT`+`UP` |
| Previous Mode | `ALT`+`DOWN` |

### Window Hotkeys
| Action | Hotkey | Notes |
| ------ | ------ | ----- |
| Open/Close Key Bindings Editor | `G` | new in v2 |
| Open/Close Pixel Inspector | `P` | new in v2 |
| Open/Close Scene Inspector | `I` | now opens a window instead of printing info to console |
| Open/Close Snapshot Viewer | `ALT` + `K` | new in v2 |
| Open/Close Status Bar | `ALT` + `S` | new in v2 |
| Open/Close Exposure Editor* | `ALT` + `X` | Previously `X` |
| Open/Close Gamma Editor* | `ALT` + `Y` | Previously `Y` |
| Open/Close Path Visualizer | `V` | |
| Open/Close Axis Display | `SHIFT` + `A` | new in v2 |

### Path Visualizer
| Action | Hotkey | Notes |
| ------ | ------ | ----- |
| Toggle On/Off | `SHIFT` + `V` | |
| Pick pixel | `LMB` | just click the pixel you want to see the ray tree for |
| Previous node | `SHIFT` + `LEFT` | selects the previous node (displays path vertex information in the path visualizer menu) |
| Next node | `SHIFT` + `RIGHT` | selects the next node (display path vertex information in the path visualizer menu) |

### Miscellaneous Hotkeys
| Action | Hotkey | Notes |
| ------ | ------ | ----- |
| Render Output Previous | `,` | |
| Render Output Next | `.` | |
| Save Image* | `CTRL`+`S` | new in v2 |
| Take Snapshot | `K` | |
| Next Snapshot | `RIGHT` | new in v2, toggle next snapshot |
| Prev Snapshot | `LEFT` | new in v2, toggle prev snapshot |
| Tile Progress Toggle* | `T` | Previously `P` |
| Print Key Bindings | `H` | prints keybindings to console |

## Configuration Files

moonray_gui_v2 stores user configuration and state files in a dedicated config directory to keep the project folder clean:

- **Linux/macOS**: `~/.config/moonray_gui/`
- **Windows**: `%APPDATA%/moonray_gui/`

The following files are stored there:
- `imgui.ini` - ImGui window layout and positions
- (Future: keybindings)

The config directory is created automatically on first run. If creation fails, settings will not persist between sessions.

For snapshots, the user provides a snapshot directory, and moonray_gui_v2 creates `snapshots.txt` in this directory. This file stores the snapshots in order, along with their assigned labels, in the format:

filename|label

