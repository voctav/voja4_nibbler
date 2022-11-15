# Nibbler - Text based emulator for Voja's 4-bit processor.

_Eats nibbles for breakfast._

## Requirements

Modern C compiler and ncurses library. Only tested on Linux.

## Basic Usage

To build:
```
make
```

To run:
```
./nibbler file.bin
```

There are some sample binary programs in `examples/`, which are assembled from
the official badge tools repo.

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

## Known Issues

Terminal colors are not reset correctly on exit. This should have a smaller
impact now that the first 8 colors are not redefined.

## Resources

  * https://hackaday.io/project/182568-badge-for-supercon6-november-2022/discussion-181045
  * https://github.com/Hack-a-Day/2022-Supercon6-Badge-Tools
