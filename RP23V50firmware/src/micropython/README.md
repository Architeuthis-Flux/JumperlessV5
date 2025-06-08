Embedded micropython
==============================================================

TODO:
- Better interaction with micropython repo, hopefully without resorting to git submodules
- Actual integration support with jumperless hardware

Pre-requisites
--------------------

Have the micropython repo cloned to `$HOME/src/micropython/micropython`

Building
--------------------

First build the embed port using:

    $ make -f micropython_embed.mk MICROPYTHON_TOP=$HOME/src/micropython/micropython

This will generate the `micropython_embed` directory which is a self-contained
copy of MicroPython suitable for embedding.  The .c files in this directory need
to be compiled into your project, in whatever way your project can do that.

