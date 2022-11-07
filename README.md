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

Keys:
  * Q - end program and exit.
  * Space - enter single step mode, or execute next instruction.
  * Enter - exit single step mode and continue running program.
  * Left/Right - decrement/increment Page register.

Note that pressing Esc results in a small delay as the getch is attempting to
parse the escape sequence.

## Features

  * Simulated LED matrix shows active page.
  * Dimmer is simulated using color output (interpolates between black and
    yellow).
  * Clock and Sync registers supported. The simulation itself is very fast and
    can do faster than 100 KHz, but UI updates may introduce slight delays.
  * All registers are mapped correctly onto user memory and can be visualized
    in the matrix just like on the real hardware.

## Missing Features

This has only been tested with a handful of programs, so expect some bugs.

The following features are not implemented yet:
  * Overflow flag (for signed arithmetic)
  * RNG
  * Keys
  * GPIO
  * UART

There a lot more work to be done on the UI.

## Resources

  * https://hackaday.io/project/182568-badge-for-supercon6-november-2022/discussion-181045
  * https://github.com/Hack-a-Day/2022-Supercon6-Badge-Tools

