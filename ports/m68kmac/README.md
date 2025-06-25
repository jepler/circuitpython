# The m68k Mac port

This port runs on m68k macs. The author tests it in a modified umac emulating a
4MB "mac plus" with System 7, though it may also run on System 6.

## Building and running

The build assumes it will occur inside the Retro68 docker image:

    $ docker run --rm --mount type=bind,source=.,destination=/work -it ghcr.io/autc04/retro68 make -C /work/ports/m68kmac

A modified version of umac with multi disc image support is required.
To run the executable and get a basic working REPL do:

    $ /path/to/umac/main -r rom.bin -d HyperCardBootSystem7.img -d build/firmware.dsk

.. then when the firmware disk is mounted, double click it and then the firmware application icon.

## Key TODOs

 * Correctly implement GC collection (stack + registers)
 * Add Mac API support (e.g., quickdraw, arbitrary traps)
 * Support larger heap
 * Address GPL files (RetroConsole), GitHub Actions CI, and other issues that might prevent upstream inclusion.
