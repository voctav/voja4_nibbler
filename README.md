# Nibbler - Text based emulator for Voja's 4-bit processor.

_Eats nibbles for breakfast._

## Building

### Requirements

Modern C compiler, make, and ncurses library (development files).

## Linux

To install ncurses library development files on Ubuntu/Debian:
```
sudo apt-get install ncurses-dev
```

To build:
```
make
```

## Windows (MSYS2)

Only MSYS2 platform was tested on Windows. Ensure `gcc` and `make` are installed
before continuing.

To install ncurses library development files:
```
pacman -S ncurses-devel
```

To build:
```
make
```

## Basic Usage

To run:
```
./nibbler file.hex
```

There are some sample binaries that work with the emulator in `examples/`.
These have been assembled from the official badge tools repo.

Keys:
  * Q - end program and exit.
  * Space - enter single step mode, or execute next instruction.
  * Enter - exit single step mode and continue running program.
  * Left/Right - decrement/increment Page register.
  * `<tab>` - key 0 (mode).
  * `1 2 3 4` - keys 1-4 (opcode).
  * `A S D F` - keys 5-8 (operand x).
  * `Z X C V` - keys 9-12 (operand y).
  * `/` - key 13 (data in).

Note that pressing `Esc` results in a small delay as `getch` is attempting to
parse the escape sequence.

## Command Line Options

  * The -s option tells nibbler to start in single step mode. The default is
    to start in free running mode and require the user pause execution with the
    space key.
  * The -r option will color the page display area red on terminals that support
    colors. This better simulates the look for the real hardware.

## Terminal Settings

Dimming is only supported for terminals with 256 colors.

It's possible to change `TERM` environment variable to force a certain terminal
type (e.g. `export TERM=xterm-256color`):
  * `xterm-mono` - monochrome.
  * `xterm-color` - 8 colors.
  * `xterm-16color` - 16 colors.
  * `xterm-256color` - 256 colors.

## Features

  * Simulated LED matrix shows active page.
  * Dimmer is simulated using color output (interpolates between black and
    yellow).
  * Clock and Sync registers supported. The simulation itself is very fast and
    can do faster than 100 KHz, but UI updates may introduce slight delays.
  * All registers are mapped correctly onto user memory and can be visualized
    in the matrix just like on the real hardware.
  * Basic support for keys. AnyPress and LastPress flags will be set the first
    time a recognized key is pressed and will stay like that. JustPress will be
    set every time a new key is recognized, and it will be reset when the
    program reads the register (as expected).

## Missing Features

The core VM seems to work well with a variety of programs, but there may still
be bugs.

The following features are not implemented yet:
  * Keys are implemented partially (no key up events)
  * GPIO
  * UART

There a lot more work to be done on the UI.

## Resources

  * https://hackaday.io/project/182568-badge-for-supercon6-november-2022/discussion-181045
  * https://github.com/Hack-a-Day/2022-Supercon6-Badge-Tools
