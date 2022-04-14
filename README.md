# Make A Fish Cropper

A GIMP plugin that crops screenshots from http://makea.fish in order to isolate the fish.

## Examples

Before:
| Before                                        | After                                        |
| :---------------------------------------------| :------------------------------------------- |
| ![Before Fish 18](/README_Images/fish_18.jpg) | ![After Fish 18](/README_Images/fish_18.png) |
| ![Before Fish 53](/README_Images/fish_53.jpg) | ![After Fish 71](/README_Images/fish_53.png) |
| ![Before Fish 71](/README_Images/fish_71.jpg) | ![After Fish 71](/README_Images/fish_71.png) |

## How it works

It's done it 7 steps

1. Find the white borders around the fish
2. Crop out the white borders
3. Add an Alpha Channel if none exists
4. Uses select contiguous color to select all of the background
5. Clear selection (i.e. clear out the background)
6. Select item ("Alpha to Selection" matches the layer option in the UI)
7. Crop to selection

## Compiling

### Linux

Make sure you have libgimp2.0-dev installed then run this to build

```bash
gimptool-2.0 --build MakeAFishCropper.c
```

Or this to build and install

```bash
gimptool-2.0 --install MakeAFishCropper.c
```

### Windows

Work in progress. One place to look would be [here](https://www.lprp.fr/2021/06/compiling-gimp-plugins-for-windows-has-never-been-so-easy-with-msys2/)

## Installing

### Linux

You can follow the setps in [Compiling](#Compiling). Alternatively, you can copy the MakeAFishCropper plugin file to `~/.config/GIMP/2.10/plug-ins` (or similar depending on your Distro).

### Windows

Work in progress. Need someone to test but it's probably in APPDATA.
