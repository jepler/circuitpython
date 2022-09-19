#Instructions for adding C module to CircuitPython build, tested on 8.00

The instructions here are based on the Adafruit MP3 library https://github.com/adafruit/Adafruit_MP3 And the check-in at https://github.com/adafruit/circuitpython/pull/2337/commits/a08d9e6d8e0059c8aa59feadd7b94d95d691e273

1. Create a CircitPython build environment per https://learn.adafruit.com/building-circuitpython

    Note: When following this guide, After running `make fetch-submodules` the repository will not include tags, resulting in unexpected version numbers in the executables. Fix this by running:

    `git fetch --tags --recurse-submodules=no --shallow-since="2021-07-01" `

    The repository will be in a **detached head** status. If you are using visual studio code it will show several modules that need to be published. These can be safely ignored.

1. Create, build, and test your C library and set it up in it's own Github repository. See ```docs/design_guide.rst``` in your fork for information about building libraries.

1.  Add your library to your fork:
    ```
    git submodule add <repo> lib/<lib>
    Example: git submodule add  https://github.com/adafruit/Adafruit_MP3 lib/mp3
    ```

1.  Edit your port's .mk and .h files to include your module:
    ```
    ports/<port>/boards/<board>/mpconfigboard.mk
        MY_LIB = 0

    ports/<port>/mpconfigport.mk
        ifndef MY_LIB
            MY_LIB = 0
        endif

    py/circuitpy_defns.mk
        ifeq ($(MY_LIB),1)
            SRC_PATTERNS += <lib>/%
        endif

        in SRC_COMMON_HAL_ALL = \
          .
          Insert alphabetically
          .
        	<lib>/__init__.c \
	        <lib>/<my_source>.c \

        Create a new section:
        ifeq ($(MY_LIB),1)
            SRC_MOD += $(addprefix lib/<lib>/src/, \
                file-1.c \
                file-1.c \
                file-n.c \
            )
            $(BUILD)/lib/<lib>/src/buffers.o: CFLAGS += -include "py/misc.h" -D'MPDEC_ALLOCATOR(x)=m_malloc(x,0)' -D'MPDEC_FREE(x)=m_free(x)'
        endif

    py/circuitpy_mpconfig.h
        #if MY_LIB
            #define MY_MODULE { MP_OBJ_NEW_QSTR(MP_QSTR_<lib>), (mp_obj_t)&my_module },
            extern const struct _mp_obj_module_t my_module;
        #else
            #define MY_MODULE
        #endif

    py/circuitpy_mpconfig.mk
        CFLAGS += -DMY_LIB=$(MY_LIB)
    ```
1. Add your .c and .h binding files per https://learn.adafruit.com/extending-circuitpython. Ignore the warning at the top and do not follow the instructions at the end about the makefile, That's been done above. The files you'll add are:

    ```
    shared-bindings/<lib>/
        <myfile>.c
        <myfile>.h
        __init__.c
        __init__.h
    shared-module/<lib>/
        <myfile>.c
        <myfile>.h
        __init__.c
        __init__.h
    ```

After all that's done, make your port.
