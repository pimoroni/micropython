sys.settrace() Local Variable Name Preservation
===============================================

This document describes the local variable name preservation feature for MicroPython's
``sys.settrace()`` functionality, which allows debuggers and profilers to access
meaningful variable names instead of just indexed values.

Overview
--------

MicroPython's ``sys.settrace()`` implementation traditionally provided access to local
variables through generic index-based names (``local_00``, ``local_01``, etc.).
The local variable name preservation feature extends this by storing and exposing
the actual variable names when available.

This feature enables:

* Enhanced debugging experiences with meaningful variable names
* Better profiling and introspection capabilities
* Improved development tools that can show actual variable names

Configuration
-------------

The feature is controlled by configuration macros:

``MICROPY_PY_SYS_SETTRACE_LOCALNAMES``
  Enables local variable name preservation for source files (RAM storage)
  and frozen bytecode (ROM storage).
  Default: ``0`` (disabled)

Dependencies
~~~~~~~~~~~~

* ``MICROPY_PY_SYS_SETTRACE`` must be enabled
* ``MICROPY_PERSISTENT_CODE_SAVE`` must be enabled

Memory Usage
~~~~~~~~~~~~

When enabled, the feature adds:

* One pointer field (``local_names``) per function in ``mp_raw_code_t``
* One length field (``local_names_len``) per function in ``mp_raw_code_t``
* One ``qstr_short_t`` array per function containing local variable names

Total memory overhead per function: ``8 bytes + (num_locals * sizeof(qstr_short_t))``.
For frozen bytecode this is all in ROM, along with any names not already
interned in the firmware.

Implementation Details
----------------------

Architecture
~~~~~~~~~~~~

The implementation consists of several components:

1. **Compilation Phase** (``py/compile.c``)
   Collects local variable names during scope analysis and stores them
   in an array allocated with the raw code object.

2. **Storage** (``py/emitglue.h``)
   Extends ``mp_raw_code_t`` structure to include local variable name storage
   with proper bounds checking.

3. **Runtime Access** (``py/profile.c``)
   Provides access to variable names through ``frame.f_locals`` in trace
   callbacks, falling back to index-based names when real names unavailable.

4. **Unified Access Layer** (``py/emitglue.h``)
   Provides ``mp_raw_code_get_local_name()`` function with bounds checking.

5. **Persistence** (``py/persistentcode.c``, ``tools/mpy-tool.py``)
   ``mpy-cross`` writes the names to an optional section of the .mpy file,
   which ``mpy-tool.py`` freezes into ROM arrays.

Data Structures
~~~~~~~~~~~~~~~

Extended ``mp_raw_code_t`` structure:

.. code-block:: c

    typedef struct _mp_raw_code_t {
        // ... existing fields ...
        #if MICROPY_PY_SYS_SETTRACE_LOCALNAMES
        const qstr_short_t *local_names; // Array of local variable names
        uint16_t local_names_len;        // Length of local_names array
        #endif
        // ... asm fields ...
    } mp_raw_code_t;

The fields come before the inline assembler fields, and are repeated in
``mp_raw_code_truncated_t``, which frozen bytecode uses, so they are at the
same offset in both.

Local Variable Name Collection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

During compilation in ``scope_compute_things()``:

1. Check if scope is function-like and has local variables
2. Allocate ``qstr_short_t`` array with size ``scope->num_locals``
3. Iterate through ``id_info`` and populate array at correct indices
4. Store array pointer and length in raw code object

.. code-block:: c

    if (SCOPE_IS_FUNC_LIKE(scope->kind) && scope->num_locals > 0) {
        qstr_short_t *names = m_new0(qstr_short_t, scope->num_locals);
        for (int i = 0; i < scope->id_info_len; i++) {
            id_info_t *id = &scope->id_info[i];
            if ((id->kind == ID_INFO_KIND_LOCAL || id->kind == ID_INFO_KIND_CELL || id->kind == ID_INFO_KIND_FREE) &&
                id->local_num < scope->num_locals) {
                names[id->local_num] = id->qst;
            }
        }
        scope->raw_code->local_names = names;
        scope->raw_code->local_names_len = scope->num_locals;
    }

Bounds Checking
~~~~~~~~~~~~~~~

Critical for memory safety, the unified access function includes bounds checking:

.. code-block:: c

    static inline qstr mp_raw_code_get_local_name(const mp_raw_code_t *rc, uint16_t local_num) {
        if (rc->local_names != NULL && local_num < rc->local_names_len) {
            return rc->local_names[local_num];
        }
        return MP_QSTRnull;
    }

.mpy Format
~~~~~~~~~~~

When ``mpy-cross`` is built with the feature, it writes a local variable names
section after the outer raw code:

* the tag byte ``L``
* for each raw code, in the order the raw codes are saved (each raw code, then
  its children): the number of names, then that many qstrs, an empty qstr for
  an unnamed slot

The section follows the point where ``mp_raw_code_load()`` stops reading, so it
needs no version change and is ignored when a .mpy file is imported.
``mpy-tool.py`` reads it when freezing and emits each function's names as a
``qstr_short_t`` array referenced from its ``mp_raw_code_t``. ``mpy-tool.py``
does not carry the names into merged .mpy files.

Usage
-----

Python API
~~~~~~~~~~

The feature integrates transparently with existing ``sys.settrace()`` usage:

.. code-block:: python

    import sys

    def trace_handler(frame, event, arg):
        if event == 'line':
            locals_dict = frame.f_locals
            print(f"Local variables: {list(locals_dict.keys())}")
        return trace_handler

    def test_function():
        username = "Alice"
        age = 25
        return username, age

    sys.settrace(trace_handler)
    result = test_function()
    sys.settrace(None)

Expected output with feature enabled:

.. code-block::

    Local variables: ['username', 'age']

Expected output with feature disabled:

.. code-block::

    Local variables: ['local_00', 'local_01']

Behavior
~~~~~~~~

**With Real Names Available:**
Variables appear with their actual names (``username``, ``age``, etc.)

**With Fallback Behavior:**
Variables appear with index-based names (``local_00``, ``local_01``, etc.)

**Mixed Scenarios:**
Some variables may have real names while others use fallback names,
depending on compilation and storage availability.

Limitations
-----------

Source Files vs .mpy Files
~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Source files: local variable names are preserved.
* Frozen bytecode: local variable names are preserved.
* .mpy files: fall back to index-based names (``local_XX``).

Compatibility
~~~~~~~~~~~~~

* **Bytecode Compatibility:** Bytecode is unchanged
* **Memory Usage:** Adds memory overhead proportional to number of local variables
* **Performance:** Minimal runtime performance impact

Deployment Scenarios
~~~~~~~~~~~~~~~~~~~~

**Development Environment:**
Enable ``MICROPY_PY_SYS_SETTRACE_LOCALNAMES`` for full debugging capabilities
with source files.

**Production Deployment:**
Disable the feature to minimize memory usage, or enable selectively based
on debugging requirements.

**.mpy Distribution:**
Code loaded from .mpy files falls back to index-based names. Frozen code keeps
its names when the .mpy files are compiled by an ``mpy-cross`` built with the
feature, the default for ``mpy-cross``.

Testing
-------

Unit Tests
~~~~~~~~~~

The feature includes comprehensive unit tests:

* ``tests/basics/sys_settrace_localnames.py`` - Basic functionality test
* ``tests/basics/sys_settrace_localnames_comprehensive.py`` - Detailed verification

Test Coverage
~~~~~~~~~~~~~

* Basic local variable access
* Nested function variables  
* Loop variable handling
* Exception handling scenarios
* Mixed real/fallback naming
* Memory safety (bounds checking)
* Integration with existing ``sys.settrace()`` functionality

Example Test
~~~~~~~~~~~~

.. code-block:: python

    def test_basic_names():
        def trace_handler(frame, event, arg):
            if frame.f_code.co_name == 'test_func':
                locals_dict = frame.f_locals
                real_names = [k for k in locals_dict.keys() if not k.startswith('local_')]
                return real_names
        
        def test_func():
            username = "test"
            return username
            
        sys.settrace(trace_handler)
        result = test_func()
        sys.settrace(None)
        # Should capture 'username' as a real variable name

Future Enhancements
-------------------

Python Accessibility
~~~~~~~~~~~~~~~~~~~~

**Goal:** Make local variable names accessible through standard Python attributes

**Potential API:**
* ``function.__code__.co_varnames`` - Local variable names tuple
* ``frame.f_code.co_varnames`` - Access in trace callbacks

Performance Optimizations
~~~~~~~~~~~~~~~~~~~~~~~~~

* Lazy loading of variable names
* Compression of name storage
* Optional name interning optimizations

Integration Points
------------------

Debugger Integration
~~~~~~~~~~~~~~~~~~~

The feature provides a foundation for enhanced debugger support:

.. code-block:: python

    class MicroPythonDebugger:
        def __init__(self):
            self.breakpoints = {}
            
        def trace_callback(self, frame, event, arg):
            if event == 'line' and self.has_breakpoint(frame):
                # Access local variables with real names
                locals_dict = frame.f_locals
                self.show_variables(locals_dict)
            return self.trace_callback

Profiler Enhancement
~~~~~~~~~~~~~~~~~~~

Profilers can provide more meaningful variable analysis:

.. code-block:: python

    class VariableProfiler:
        def profile_function(self, func):
            def trace_wrapper(frame, event, arg):
                if event == 'return':
                    locals_dict = frame.f_locals
                    self.analyze_variable_usage(locals_dict)
                return trace_wrapper
            
            sys.settrace(trace_wrapper)
            result = func()
            sys.settrace(None)
            return result

Contributing
------------

Development Guidelines
~~~~~~~~~~~~~~~~~~~~~~

When modifying the local variable name preservation feature:

1. **Memory Safety:** Always include bounds checking for array access
2. **Compatibility:** Maintain bytecode compatibility
3. **Testing:** Add tests for new functionality
4. **Documentation:** Update this documentation for any API changes

Code Review Checklist
~~~~~~~~~~~~~~~~~~~~~

* ✅ Bounds checking implemented for all array access
* ✅ Memory properly allocated and freed
* ✅ Configuration macros respected
* ✅ Fallback behavior maintains compatibility
* ✅ Unit tests added for new functionality
* ✅ Documentation updated

File Locations
~~~~~~~~~~~~~~

**Core Implementation:**
* ``py/compile.c`` - Local name collection during compilation
* ``py/emitglue.h`` - Data structures and unified access
* ``py/emitglue.c`` - Initialization
* ``py/profile.c`` - Runtime access through ``frame.f_locals``
* ``py/mpconfig.h`` - Configuration macros

**Testing:**
* ``tests/basics/sys_settrace_localnames.py`` - Unit tests
* ``tests/basics/sys_settrace_localnames_comprehensive.py`` - Integration tests

**Documentation:**
* ``docs/develop/sys_settrace_localnames.rst`` - This document