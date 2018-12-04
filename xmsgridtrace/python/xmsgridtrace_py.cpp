//------------------------------------------------------------------------------
/// \file
/// \brief root module for xmsgrid Python bindings.
/// \copyright (C) Copyright Aquaveo 2018. Distributed under FreeBSD License
/// (See accompanying file LICENSE or https://aqaveo.com/bsd/license.txt)
//------------------------------------------------------------------------------

//----- Included files ---------------------------------------------------------
#include <pybind11/pybind11.h>
#include <xmsgridtrace/python/gridtrace/gridtrace_py.h>

//----- Namespace declaration --------------------------------------------------
namespace py = pybind11;

//----- Python Interface -------------------------------------------------------
#ifndef XMS_VERSION
  #define XMS_VERSION "99.99.99";
#endif


//------ Primary Module --------------------------------------------------------
PYBIND11_MODULE(xmsgridtrace, m) {
    m.doc() = "Python bindings for xmsgridtrace"; // optional module docstring
    m.attr("__version__") = XMS_VERSION;

    const char* misc_doc = R"pydoc(
        The gridtrace module of the xmsgridtrace python library contains classes
        and functions for tracing the movement of a point through a scalar grid.
        The gridtrace module contains the functionality to create the flow trace
        of a point between 2 velocity vector time steps on an XmGrid. The module
        supports velocity vectors assigned to either points or cells.
    )pydoc";      

    // Extractor module
    py::module modGridtrace = m.def_submodule("gridtrace", misc_doc);
    initGridtrace(modGridtrace);
}

