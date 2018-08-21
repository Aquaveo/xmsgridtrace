set(CMAKE_CXX_STANDARD 11)
set(POSITION_INDEPENDENT_CODE True)
cmake_minimum_required(VERSION 3.1.2)
cmake_policy(SET CMP0015 NEW) # Link Directory Pathing
set(CMAKE_DEBUG_POSTFIX _d)

set(BUILD_TESTING NO CACHE BOOL "Enable/Disable testing")
set(IS_CONDA_BUILD NO CACHE BOOL "Set this if you want to make a conda package.")
set(CONDA_PREFIX "" CACHE PATH "Path to the conda environment used to build.")
set(IS_PYTHON_BUILD NO CACHE BOOL "Set this if you want to build the python bindings.")
set(XMSGRIDTRACE_TEST_PATH "../../test_files/" CACHE PATH "Path to test files for testing")

project(xmsgridtrace C CXX)

if(WIN32)
    if(XMS_BUILD)
        add_definitions(/D _WIN32_WINNT=0x0501)  # Windows XP and higher
        add_definitions(/Zc:wchar_t-)  # Treat wchar_t as built-in type
    else(NOT XMS_BUILD)
        add_definitions(/D BOOST_ALL_NO_LIB)
    endif()
endif()

if(IS_CONDA_BUILD)
  include(${CMAKE_CURRENT_LIST_DIR}/condabuildinfo.cmake)
else() # If we are not using conda, we are using conan
  # Conan setup
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup(TARGETS)
  set(EXT_INCLUDE_DIRS ${CONAN_INCLUDE_DIRS})
  set(EXT_LIB_DIRS ${CONAN_LIB_DIRS})
  set(EXT_LIBS ${CONAN_LIBS})
endif(IS_CONDA_BUILD)

message("External Include Dirs: ${EXT_INCLUDE_DIRS}")
message("External Lib Dirs: ${EXT_LIB_DIRS}")
message("Extneral Libs: ${EXT_LIBS}")

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${EXT_INCLUDE_DIRS})
link_directories(${EXT_LIB_DIRS})

# Static library sources
set(xmsgridtrace_sources
  xmsgridtrace/gridtrace/XmGridTrace.cpp
  xmsgridtrace/gridtrace/XmUGrid2dDataExtractor.cpp
  xmsgridtrace/gridtrace/XmUGrid2dTriangles2d.cpp
)

set(xmsgridtrace_headers
  xmsgridtrace/gridtrace/XmGridTrace.h
  xmsgridtrace/gridtrace/XmUGrid2dDataExtractor.h
  xmsgridtrace/gridtrace/XmUGrid2dTriangles2d.h
)

# Pybind11 sources
set(xmsgridtrace_py
  xmsgridtrace/python/xmsgridtrace_py.cpp
  #GridTrace
  xmsgridtrace/python/gridtrace/gridtrace_py.cpp
  xmsgridtrace/python/gridtrace/XmGridTrace_py.cpp
)

set(xmsgridtrace_py_headers
  xmsgridtrace/python/gridtrace/gridtrace_py.h
)

# Tests
if (BUILD_TESTING)
 add_definitions(-DXMSGRIDTRACE_TEST_PATH="${XMSGRIDTRACE_TEST_PATH}/")
 add_definitions(-DCXX_TEST -DCXXTEST4)

 list(APPEND xmsgridtrace_sources
   xmsgridtrace/gridtrace/XmGridTrace.t.h
 )

 find_package(CxxTest)
 if(CXXTEST_FOUND)
   include_directories(${CXXTEST_INCLUDE_DIR})
   enable_testing()

   set(CXXTEST_TESTGEN_ARGS --xunit-printer --have-eh)
   file(GLOB_RECURSE test_headers ${CMAKE_CURRENT_LIST_DIR}/xmsgridtrace/*.t.h)
   CXXTEST_ADD_TEST(
     runner runner.cpp ${test_headers}
   )
   target_link_libraries(runner ${PROJECT_NAME})
 endif()
endif ()

# Static library
add_library(${PROJECT_NAME} STATIC
  ${xmsgridtrace_sources} ${xmsgridtrace_headers}
)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/>
    ${Boost_INCLUDE_DIR}
 )
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME}
  ${EXT_LIBS}
)
target_link_libraries(${PROJECT_NAME}
  ${CMAKE_THREAD_LIBS_INIT}
)
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} rt)
endif()

if(IS_PYTHON_BUILD)
    find_package(PythonLibs REQUIRED)
    message("PYTHON_INCLUDE: ${PYTHON_INCLUDE_DIRS}")
    message("PYTHON_LIBS: ${PYTHON_LIBRARIES}")

    # Pybind11 module
    if(IS_CONDA_BUILD)
        include("${CONDA_PREFIX}/share/cmake/pybind11/pybind11Targets.cmake")
        include("${CONDA_PREFIX}/share/cmake/pybind11/FindPythonLibsNew.cmake")
        include("${CONDA_PREFIX}/share/cmake/pybind11/pybind11Config.cmake")
        include("${CONDA_PREFIX}/share/cmake/pybind11/pybind11ConfigVersion.cmake")
        include("${CONDA_PREFIX}/share/cmake/pybind11/pybind11Tools.cmake")

    else()
        include("${CONAN_PYBIND11_ROOT}/share/cmake/pybind11/pybind11Targets.cmake")
        include("${CONAN_PYBIND11_ROOT}/share/cmake/pybind11/FindPythonLibsNew.cmake")
        include("${CONAN_PYBIND11_ROOT}/share/cmake/pybind11/pybind11Config.cmake")
        include("${CONAN_PYBIND11_ROOT}/share/cmake/pybind11/pybind11ConfigVersion.cmake")
        include("${CONAN_PYBIND11_ROOT}/share/cmake/pybind11/pybind11Tools.cmake")
    endif()

    pybind11_add_module(xmsgridtrace_py
      ${xmsgridtrace_py} ${xmsgridtrace_py_headers}
    )
    target_include_directories(xmsgridtrace_py
        PRIVATE
          ${EXT_LIBS}
          PYTHON_INCLUDE_DIRS
    )
    target_link_libraries(xmsgridtrace_py
      PRIVATE
        ${EXT_LIBS}
        ${PROJECT_NAME}
    )
    set_target_properties(xmsgridtrace_py PROPERTIES
      LINKER_LANGUAGE CXX
    )

    execute_process(COMMAND python -c "import os; print('${PYTHON_SITE_PACKAGES}'.replace('${CONDA_PREFIX}' + os.sep, ''))" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES_TRIMMED OUTPUT_STRIP_TRAILING_WHITESPACE)

    # Install recipe
    install(
      TARGETS xmsgridtrace_py
      ARCHIVE DESTINATION ${PYTHON_SITE_PACKAGES_TRIMMED}
      LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES_TRIMMED}
    )

endif()


# Install recipe
install(
  TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)
foreach (header IN LISTS xmsgridtrace_headers xmsgridtrace_py_headers)
  get_filename_component(subdir "${header}" DIRECTORY)
  install(
    FILES "${header}"
    DESTINATION "include/${subdir}"
  )
endforeach ()
