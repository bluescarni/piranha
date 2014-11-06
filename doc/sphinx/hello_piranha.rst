.. _hello_piranha:

Hello Piranha!
==============

Your first Piranha program
--------------------------

After following the :ref:`installation instructions <getting_started>`,
you should be able to compile and run your first C++ Piranha program:

.. literalinclude:: ../../tutorial/hello_piranha.cpp
   :language: c++
   :linenos:

.. note:: The C++ code snippets shown in the tutorial are imported verbatim from the Piranha
   source tree. You should take care of adjusting the include path of the ``piranha.hpp`` header
   to the correct location on your system before attempting to compile the C++ code samples.
   On Unix systems, if you installed Piranha in a standard ``PREFIX``, the correct way
   of including the Piranha header is ``#include <piranha/piranha.hpp>``.

This program will just print to screen the rational number :math:`\frac{4}{3}`, represented
in Piranha by a class called (unsurprisingly) ``rational``.
In Unix-like environments, you can compile this simple program with GCC via the command:

.. code-block:: bash

   $ g++ -std=c++11 hello_piranha.cpp -lmpfr -lgmp

A couple of things to note:

* we pass the ``-std=c++11`` flag to specify that we want to use the C++11 version of the C++ standard for the compilation.
  Piranha is written in C++11, and this flag is necessary as long as GCC does not default to C++11 mode;
* we specify via the ``-lmpfr -lgmp`` flags that the executable needs to be linked to the GMP and MPFR libraries (if
  you do not do this, the program will still compile but the final linking will fail due to undefined references);
* at the present time, all the Boost libraries used within Piranha are header-only and thus no linking to any Boost
  library is necessary;
* Piranha itself is header-only, so there is no ``-lpiranha`` to link to.

.. note:: The development version of Piranha adds serialization capabilities, and requires the corresponding
   Boost library to be linked in with ``-lboost_serialization`` if such capablities are used.

If you installed Piranha in a custom ``PREFIX``, you will need to specify on the command line where
the Piranha headers are located via the ``-I`` switch. E.g.,

.. code-block:: bash

   $ g++ -I/home/username/.local/include -std=c++11 hello_piranha.cpp -lmpfr -lgmp

If the GMP and/or MPFR libraries are not installed in a standard path, you can use the ``-L`` switch to tell GCC
where to look for them:

.. code-block:: bash

   $ g++ -std=c++11 hello_piranha.cpp -L/custom/library/path -lmpfr -lgmp
