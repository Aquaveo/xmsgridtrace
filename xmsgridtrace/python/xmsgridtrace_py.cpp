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
std::string version() {
    return "1.0.0";
}


//------ Primary Module --------------------------------------------------------
PYBIND11_MODULE(xmsgridtrace_py, m) {
    m.doc() = "Python bindings for xmsgridtrace"; // optional module docstring
    m.def("version", &version,
          "Get current version of xmsgridtrace Python bindings.");

    const char* misc_doc = R"pydoc(
        The misc module of the xmsgridtrace python library contains classes and
        functions that are shared between all of the xms family of libraries.
        These functions and classes can be used in any of the library to ensure
        that functionality is standardized. Xmsgridtrace includes the following 
        functionality, the flow trace of a point between 2 velocity vector time 
        steps on an XmGrid can be calculated and the velocity vectors can be 
        assigned to either the points or to the cells.
    )pydoc";      

    // Extractor module
    py::module modGridtrace = m.def_submodule("gridtrace", misc_doc);
    initGridtrace(modGridtrace);
}

