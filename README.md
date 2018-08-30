# Bim - A Bad IMitation of Vi(m)

![screenshot](screenshot.png)

Bim is a terminal text editor with syntax highlighting.

Inspired by Vim and featuring similar mode-based editing, Bim was originally written for ToaruOS, but should work on most Unix-like operating systems with minimal modification.

## Goals / Purpose

Bim does not seek to improve or replace vim, or any other text editor. Its goal is to provide ToaruOS-NIH - a hobby operating system built completely from scratch - with a featureful editor. The ability to build and run Bim on other platform is a secondary feature.

## Features

- Vim-like modal interactions.
- Arrow-key and traditional vi hjkl navigation.
- Syntax highlighting (currently for C, Python, and Makefiles).
- Themes, including 256-color and 24-bit color support.
- Indentation adjustment.
- Multiple editor tabs.
- Basic Unicode support (sufficient for things like Japanese, but not capable of more complex scripts).
- Efficient screen redrawing.
- Terminal support tested in ToaruOS, Sortix, xterm, urxvt, Gnome, XFCE, Linux and FreeBSD consoles, macOS Terminal.app, iTerm2.
- Mouse support in Xterm-like terminals.
- Line selection and yanking.
- Incremental forward search.
- Undo/redo stack (experimental, enable with `-O history`)

## Prerequisites

Bim has no external dependencies beyond a functioning C library and sufficient escape code support in the hosting terminal.

### Terminal Support

Unicode support is recommended, but not completely required.

256-color and 24-bit color are optional. The default theme uses only the standard 16 colors.

Scrolling is normally done through `^[[1S` and `^[[1T`, but this can be disabled with the `:noscroll` command.

Mouse support with `^[[?1000h` is available. The alternate screen is used if available with `^[[?1049h`.

## License

Bim is primarily distributed under the ISC license. In ToaruOS, it is under the NCSA license. The terms are nearly identical from a legal standpoint.

## Development

Bim is still primarily developed alongside ToaruOS-NIH. This repository is a mirror with fake history going back to the start of that project. Pull requests merged here will be patched into [ToaruOS-NIH](https://git.toaruos.org/klange/toaru-nih).
