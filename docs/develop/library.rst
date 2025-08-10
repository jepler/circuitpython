.. _internals_library:

Implementing a Module
=====================

This chapter details how to implement a core module in MicroPython.
MicroPython modules can be one of the following:

- Built-in module: A general module that is be part of the MicroPython repository.
- User module: A module that is useful for your specific project that you maintain
  in your own repository or private codebase.
- Dynamic module: A module that can be deployed and imported at runtime to your device.

A module in MicroPython can be implemented in one of the following locations:

- py/: A core library that mirrors core CPython functionality.
- extmod/: A CPython or MicroPython-specific module that is shared across multiple ports.
- ports/<port>/: A port-specific module.

.. note::
   This chapter describes modules implemented in ``py/`` or core modules.
   See :ref:`extendingmicropython` for details on implementing an external module.
   For details on port-specific modules, see :ref:`porting_to_a_board`.

Implementing a core module
--------------------------

Like CPython, MicroPython has core builtin modules that can be accessed through import statements.
An example is the ``gc`` module discussed in :ref:`memorymanagement`.

.. code-block:: bash

   >>> import gc
   >>> gc.enable()
   >>>

MicroPython has several other builtin standard/core modules like ``io``, ``array`` etc.
Adding a new core module involves several modifications.

First, create the ``C`` file in the ``py/`` directory. In this example we are adding a
hypothetical new module ``subsystem`` in the file ``modsubsystem.c``:

.. code-block:: c

   #include "py/builtin.h"
   #include "py/runtime.h"
   #include "py/romtable.h"

   #if MICROPY_PY_SUBSYSTEM

   // info()
   static mp_obj_t py_subsystem_info(void) {
       return MP_OBJ_NEW_SMALL_INT(42);
   }
   MP_DEFINE_CONST_FUN_OBJ_0(subsystem_info_obj, py_subsystem_info);

   MP_ROM_TABLE(static, mp_module_subsystem_globals_table,
       ((MP_QSTR___name__, MP_ROM_QSTR, MP_QSTR_subsystem))
       ((MP_QSTR_info, MP_ROM_PTR, &subsystem_info_obj))
   );
   static MP_DEFINE_CONST_DICT(mp_module_subsystem_globals, mp_module_subsystem_globals_table);

   const mp_obj_module_t mp_module_subsystem = {
       .base = { &mp_type_module },
       .globals = (mp_obj_dict_t *)&mp_module_subsystem_globals,
   };

   MP_REGISTER_MODULE(MP_QSTR_subsystem, mp_module_subsystem);

   #endif

The implementation includes a definition of all functions related to the module and adds the
functions to the module's global table in ``mp_module_subsystem_globals_table``. It also
creates the module object with ``mp_module_subsystem``.  The module is then registered with
the wider system via the ``MP_REGISTER_MODULE`` macro.

After building and running the modified MicroPython, the module should now be importable:

.. code-block:: bash

   >>> import subsystem
   >>> subsystem.info()
   42
   >>>

Our ``info()`` function currently returns just a single number but can be extended
to do anything.  Similarly, more functions can be added to this new module.

Rom tables
----------

Rom tables are introduced with the ``MP_ROM_TABLE`` macro.
The format of the arguments to ``MP_ROM_TABLE`` are unusual due to the requirements of the boost preprocessor library.
This method is used instead of the traditional method of directly declaring an array of ``mp_rom_map_elem_t`` to allow for future flexibility.
Namely, it would save ROM space if the ROM qstr keys could be stored as ``qstr_short_t`` objects without extra padding.

The macro invocation has 3 arguments:
 * The storage class for the created table. This is usually "static".
 * The name of the created table itself.
 * A series of doubly parenthesized table entries (which lack trailing commas, making them a single macro argument).

Each table entry consists of 3 arguments:
 * An ``MP_QSTR_`` naming the table entry (without ``MP_ROM_QSTR`` or similar)
 * A macro name such as MP_ROM_PTR or MP_OBJ_ROM_QSTR which says how to transform the value
 * The value itself

Note that none of these may themselves contain commas. In fact, the way that ``MP_ROM_PTR(&obj)`` _can_
contain commas in its expansion is precisely why the 3-argument version is used.

For example, in the code above the first key is ``MP_STR___name__``, its value
is ``MP_STR_subsystem``, and the value-transforming macro is ``MP_OBJ_ROM_QSTR`.
