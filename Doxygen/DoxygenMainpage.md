xmsgridtrace {#mainpage}
============
xmsgridtrace {#mainpage}
============
\tableofcontents

xmsgridtrace {#xmsgridtrace}
============

Introduction {#Introduction}
------------

xmsgridtrace is a cross-platform C++ software library written by [Aquaveo](http://www.aquaveo.com/) in partnership with [ERDC](http://www.erdc.usace.army.mil/). The code can be obtained from github.com.

<table align="center" border="0">
  <tr>
    <td>![](Aquaveo_Logo.png)</td>
  </tr>
</table>

License {#License}
-------

The code is distributed under the xmsng Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at [http://www.aquaveo.com/xmsng/LICENSE_1_0.txt](http://www.aquaveo.com/xmsng/LICENSE_1_0.txt)). 

Python
------

This library is available as a python module. See [Python Documentation](./pydocs)

Features {#Features}
--------
xmsgridtrace includes the following functionality:

* Flow trace of a point between 2 velocity vector time steps on an XmGrid.
* The velocity vectors can be assigned to either the points or to the cells.

The following tutorial is available:

* [Gridrace tutorial](Gridtrace_Tutorial.md)

Testing {#XmsgridtraceTesting}
-------

The code has numerous unit tests which use the [CxxTest](http://cxxtest.com/) framework. A good way to see how to use the code is to look at the unit tests. Unit tests are located at the bottom of .cpp files within a "#if CXX_TEST" code block. Header files that are named with ".t.h" contain the test suite class definitions.

The Code {#XmsgridtraceTheCode}
--------
### Namespaces {#XmsgridtraceNamespaces}
* "xms" - Most code is in this namespace. The use of other namespaces is kept to a minimum. Two-letter prefixes are used as "pseudo-namespaces" in code modules. Preprocessor macros typically start with "XM_" to prevent name collisions.
* "xmt" - Testing code will be put into this namespace once CXX_TEST is upgraded such that it will find the testing code in this namespace.

### Interface pattern {#XmsgridtraceInterfacePattern}
Many classes follow the interface pattern. An abstract base class is used to define the interface and a concrete implementation class is used to implement the functionality. The implementation class will be named the same as the interface class but will end in "Impl" and will only be found in the .cpp file. For example: xms::InterpIdw and xms::InterpIdwImpl. The Doxygen documentation will be for the Impl class.

More about the interface pattern can be found at the following resources:
* The C++ Programming Language, Stroustroup, p 318 - 322
* Effective C++ Third Edition, Meyers, items 31 and 40
* More Effective C++, Meyers, item 33
* Large-Scale C++ Software Design, Lakos, 6.4.1

"xmsgridtrace" Name {#xmsgridtraceName}
------------
The name "xmsgridtrace" comes from the "XMS" (GMS, SMS, WMS) water modeling software created by Aquaveo.
