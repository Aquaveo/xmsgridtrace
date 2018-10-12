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

    // Extractor module
    py::module modGridtrace = m.def_submodule("gridtrace");
    initGridtrace(modGridtrace);
}

