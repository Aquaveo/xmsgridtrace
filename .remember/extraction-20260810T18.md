# Session Context — 2026-08-10T18:36:51.462460+00:00
Branch: worktree-xmsconan-migration

## Commits
41a856d Migrate build and CI to xmsconan 2.15.2; bump xmsextractor to 10.0.6
b5c5ecf fix install script
f7e31c3 update docs
f37fd42 remove build folder
7d6a009 fix typo in pip install for sphinx
762a782 adding html files to examples folder
6f56c49 Copy examples to doxygen
e3abad3 Update conanfile.py
f607704 -clean up examples directory
7480acd -added default values for class members in XmGridTraceImpl -updated example notebooks

## Files Changed
.appveyor.yml                                                                                  |    60 -
 .github/workflows/XmsGridtrace-CI.yaml                                                         |   476 +
 .gitignore                                                                                     |    33 +
 .travis.yml                                                                                    |   151 -
 .travis/install.sh                                                                             |    25 -
 .travis/run.sh                                                                                 |    13 -
 CMakeLists.txt                                                                                 |   186 -
 Doxygen/xmsgridtrace.tag                                                                       |     6 +-
 README.md                                                                                      |     4 +-
 _package/tests/XmGridTrace_pyt.py                                                              |   634 +
 _package/tests/__init__.py                                                                     |     1 +
 _package/xms/gridtrace/__init__.py                                                             |     3 +
 _package/xms/gridtrace/grid_trace.py                                                           |   152 +
 build.py                                                                                       |    65 -
 build.toml                                                                                     |    37 +
 build_py/.vs/xmsgridtrace/v14/.suo                                                             |   Bin 28160 -> 0 bytes
 build_py/ALL_BUILD.vcxproj                                                                     |   122 -
 build_py/ALL_BUILD.vcxproj.filters                                                             |     5 -
 build_py/CMakeCache.txt                                                                        |   371 -
 build_py/CMakeFiles/3.12.1/CMakeCCompiler.cmake                                                |    73 -
 build_py/CMakeFiles/3.12.1/CMakeCXXCompiler.cmake                                              |    76 -
 build_py/CMakeFiles/3.12.1/CMakeDetermineCompilerABI_C.bin                                     |   Bin 49664 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CMakeDetermineCompilerABI_CXX.bin                                   |   Bin 49664 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CMakeRCCompiler.cmake                                               |     6 -
 build_py/CMakeFiles/3.12.1/CMakeSystem.cmake                                                   |    15 -
 build_py/CMakeFiles/3.12.1/CompilerIdC/CMakeCCompilerId.c                                      |   623 -
 build_py/CMakeFiles/3.12.1/CompilerIdC/CompilerIdC.vcxproj                                     |    68 -
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/CL.command.1.tlog                |   Bin 738 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/CL.read.1.tlog                   |   Bin 552 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/CL.write.1.tlog                  |   Bin 456 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/CompilerIdC.lastbuildstate       |     2 -
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.delete.1.tlog         |   Bin 1118 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.delete.110.tlog       |   Bin 560 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.delete.27.tlog        |   Bin 560 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.delete.51.tlog        |   Bin 560 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.delete.68.tlog        |   Bin 560 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.delete.82.tlog        |   Bin 560 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.1.tlog           |   Bin 12280 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.104.tlog         |   Bin 380 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.106.tlog         |   Bin 380 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.110.tlog         |   Bin 264 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.27.tlog          |   Bin 2654 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.43.tlog          |   Bin 35960 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.49.tlog          |   Bin 758 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.51.tlog          |   Bin 642 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.52.tlog          |   Bin 380 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.63.tlog          |   Bin 380 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.65.tlog          |   Bin 380 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.68.tlog          |   Bin 264 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.76.tlog          |   Bin 380 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.82.tlog          |   Bin 264 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.read.90.tlog          |   Bin 380 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.write.1.tlog          |   Bin 1340 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.write.110.tlog        |   Bin 264 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.write.27.tlog         |   Bin 526 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.write.43.tlog         |   Bin 264 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.write.51.tlog         |   Bin 264 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.write.68.tlog         |   Bin 264 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link-VCTIP.write.82.tlog         |   Bin 264 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link.command.1.tlog              |   Bin 1060 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link.read.1.tlog                 |   Bin 3398 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdC/Debug/CompilerIdC.tlog/link.write.1.tlog                |   Bin 450 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdCXX/CMakeCXXCompilerId.cpp                                |   602 -
 build_py/CMakeFiles/3.12.1/CompilerIdCXX/CompilerIdCXX.vcxproj                                 |    68 -
 build_py/CMakeFiles/3.12.1/CompilerIdCXX/Debug/CompilerIdCXX.tlog/CL.command.1.tlog            |   Bin 762 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdCXX/Debug/CompilerIdCXX.tlog/CL.read.1.tlog               |   Bin 564 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdCXX/Debug/CompilerIdCXX.tlog/CL.write.1.tlog              |   Bin 476 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdCXX/Debug/CompilerIdCXX.tlog/CompilerIdCXX.lastbuildstate |     2 -
 build_py/CMakeFiles/3.12.1/CompilerIdCXX/Debug/CompilerIdCXX.tlog/link.command.1.tlog          |   Bin 1084 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdCXX/Debug/CompilerIdCXX.tlog/link.read.1.tlog             |   Bin 3414 -> 0 bytes
 build_py/CMakeFiles/3.12.1/CompilerIdCXX/Debug/CompilerIdCXX.tlog/link.write.1.tlog            |   Bin 466 -> 0 bytes
 build_py/CMakeFiles/3.12.1/VCTargetsPath.txt                                                   |     1 -
 build_py/CMakeFiles/3.12.1/VCTargetsPath.vcxproj                                               |    27 -
 build_py/CMakeFiles/3.12.1/x64/Debug/VCTargetsPath.tlog/VCTargetsPath.lastbuildstate           |     2 -
 build_py/CMakeFiles/6196f5a7ae7f29708a2833f8c9b868c8/INSTALL_force.rule                        |     1 -
 build_py/CMakeFiles/6196f5a7ae7f29708a2833f8c9b868c8/generate.stamp.rule                       |     1 -
 build_py/CMakeFiles/CMakeError.log                                                             |    38 -
 build_py/CMakeFiles/CMakeOutput.log                                                            |   435 -
 build_py/CMakeFiles/TargetDirectories.txt                                                      |     5 -
 build_py/CMakeFiles/cmake.check_cache                                                          |     1 -
 build_py/CMakeFiles/feature_tests.bin                                                          |   Bin 51712 -> 0 bytes
 build_py/CMakeFiles/feature_tests.c                                                            |    20 -
 build_py/CMakeFiles/feature_tests.cxx                                                          |   398 -
 build_py/CMakeFiles/generate.stamp                                                             |     1 -
 build_py/CMakeFiles/generate.stamp.depend                                                      |    43 -
 build_py/CMakeFiles/generate.stamp.list                                                        |     1 -
 build_py/INSTALL.vcxproj                                                                       |   220 -
 build_py/INSTALL.vcxproj.filters                                                               |    13 -
 build_py/ZERO_CHECK.vcxproj                                                                    |   166 -
 build_py/ZERO_CHECK.vcxproj.filters                                                            |    13 -
 build_py/cmake_install.cmake                                                                   |    76 -
 build_py/conanbuildinfo.cmake                                                                  |  1014 --
 build_py/conanbuildinfo.txt                                                                    |   522 -
 build_py/conaninfo.txt                                                                         |    96 -
 build_py/lib/xmsgridtrace_py.exp                                                               |   Bin 787 -> 0 bytes
 build_py/x64/Release/ALL_BUILD/ALL_BUILD.log                                                   |     1 -
 build_py/x64/Release/ALL_BUILD/ALL_BUILD.tlog/ALL_BUILD.lastbuildstate                         |     2 -
 build_py/x64/Release/ZERO_CHECK/ZERO_CHECK.log                                                 |    57 -
 build_py/x64/Release/ZERO_CHECK/ZERO_CHECK.tlog/ZERO_CHECK.lastbuildstate                      |     2 -
 build_py/x64/Release/ZERO_CHECK/ZERO_CHECK.tlog/custombuild.command.1.tlog                     |   Bin 1182 -> 0 bytes
 build_py/x64/Release/ZERO_CHECK/ZERO_CHECK.tlog/custombuild.read.1.tlog                        |   Bin 19622 -> 0 bytes
 build_py/x64/Release/ZERO_CHECK/ZERO_CHECK.tlog/custombuild.write.1.tlog                       |   Bin 424 -> 0 bytes
 build_py/xmsgridtrace.VC.db                                                                    |   Bin 28512256 -> 0 bytes
 build_py/xmsgridtrace.dir/Release/xmsgridtrace.log                                             |     1 -
 build_py/xmsgridtrace.dir/Release/xmsgridtrace.tlog/CL.command.1.tlog                          |   Bin 3204 -> 0 bytes
 build_py/xmsgridtrace.dir/Release/xmsgridtrace.tlog/CL.read.1.tlog                             |   Bin 70548 -> 0 bytes
 build_py/xmsgridtrace.dir/Release/xmsgridtrace.tlog/CL.write.1.tlog                            |   Bin 386 -> 0 bytes
 build_py/xmsgridtrace.dir/Release/xmsgridtrace.tlog/Lib-link.read.1.tlog                       |   Bin 570 -> 0 bytes
 build_py/xmsgridtrace.dir/Release/xmsgridtrace.tlog/Lib-link.write.1.tlog                      |   Bin 368 -> 0 bytes
 build_py/xmsgridtrace.dir/Release/xmsgridtrace.tlog/custombuild.command.1.tlog                 |   Bin 984 -> 0 bytes
 build_py/xmsgridtrace.dir/Release/xmsgridtrace.tlog/custombuild.read.1.tlog                    |   Bin 19374 -> 0 bytes
 build_py/xmsgridtrace.dir/Release/xmsgridtrace.tlog/custombuild.write.1.tlog                   |   Bin 308 -> 0 bytes
 build_py/xmsgridtrace.dir/Release/xmsgridtrace.tlog/lib.command.1.tlog                         |   Bin 534 -> 0 bytes
 build_py/xmsgridtrace.dir/Release/xmsgridtrace.tlog/xmsgridtrace.lastbuildstate                |     2 -
 build_py/xmsgridtrace.sln                                                                      |    81 -
 build_py/xmsgridtrace.vcxproj                                                                  |   275 -
 build_py/xmsgridtrace.vcxproj.filters                                                          |    24 -
 build_py/xmsgridtrace_py.dir/Release/xmsgridtrace_py.log                                       |     1 -
 build_py/xmsgridtrace_py.dir/Release/xmsgridtrace_py.tlog/CL.command.1.tlog                    |   Bin 26466 -> 0 bytes
 build_py/xmsgridtrace_py.dir/Release/xmsgridtrace_py.tlog/CL.read.1.tlog                       |   Bin 96684 -> 0 bytes
 build_py/xmsgridtrace_py.dir/Release/xmsgridtrace_py.tlog/CL.write.1.tlog                      |   Bin 1218 -> 0 bytes
 build_py/xmsgridtrace_py.dir/Release/xmsgridtrace_py.tlog/link.command.1.tlog                  |   Bin 9142 -> 0 bytes
 build_py/xmsgridtrace_py.dir/Release/xmsgridtrace_py.tlog/link.read.1.tlog                     |   Bin 9158 -> 0 bytes
 build_py/xmsgridtrace_py.dir/Release/xmsgridtrace_py.tlog/link.write.1.tlog                    |   Bin 838 -> 0 bytes
 build_py/xmsgridtrace_py.dir/Release/xmsgridtrace_py.tlog/xmsgridtrace_py.lastbuildstate       |     2 -
 build_py/xmsgridtrace_py.dir/Release/xmsgridtrace_py.tlog/xmsgridtrace_py.write.1u.tlog        |   Bin 508 -> 0 bytes
 build_py/xmsgridtrace_py.vcxproj                                                               |   284 -
 build_py/xmsgridtrace_py.vcxproj.filters                                                       |    27 -
 conanfile.py                                                                                   |   129 -
 examples/GridtraceRealData.html                                                                | 15333 +++++++++++++++++
 examples/GridtraceRealData.ipynb                                                               |  3540 ----
 examples/GridtraceRealData.zip                                                                 |   Bin 0 -> 2275731 bytes
 examples/GridtraceTutorial.html                                                                | 15409 +++++++++++++++++
 examples/GridtraceTutorial.ipynb                                                               |  3536 ----
 examples/GridtraceTutorial.zip                                                                 |   Bin 0 -> 1261577 bytes
 examples/gridtrace_tools.py                                                                    |    93 -
 examples/particle_trace_data.py                                                                | 88439 -----------------------------------------------------------------------------------------------
 examples/xms.yml                                                                               |    24 +
 generateDocumentationAndDeploy.sh                                                              |     5 +-
 pydocs/source/conf.py                                                                          |     7 +-
 pydocs/source/getting_started.rst                                                              |    21 +
 pydocs/source/gettingstarted.rst                                                               |    17 -
 pydocs/source/index.rst                                                                        |    41 +-
 pydocs/source/modules.rst                                                                      |    18 -
 pydocs/source/modules/gridtrace/GridTrace.rst                                                  |     6 +
 pydocs/source/modules/gridtrace/XmGridTrace.rst                                                |     6 -
 test_package/CMakeLists.txt                                                                    |    11 +-
 test_package/conanfile.py                                                                      |    31 +-
 xmsgridtrace/gridtrace/XmGridTrace.cpp                                                         |    40 +-
 xmsgridtrace/python/gridtrace/XmGridTrace_pyt.py                                               |   618 -
 xmsgridtrace/python/xmsgridtrace_py.cpp                                                        |     2 +-
 151 files changed, 32211 insertions(+), 102847 deletions(-)

## New Files Created
.github/workflows/XmsGridtrace-CI.yaml
_package/tests/XmGridTrace_pyt.py
_package/tests/__init__.py
_package/xms/gridtrace/__init__.py
_package/xms/gridtrace/grid_trace.py
build.toml
examples/GridtraceRealData.html
examples/GridtraceRealData.zip
examples/GridtraceTutorial.html
examples/GridtraceTutorial.zip
examples/xms.yml
pydocs/source/getting_started.rst
pydocs/source/modules/gridtrace/GridTrace.rst

## Untracked Files
.remember/extraction-20260810T18.md
