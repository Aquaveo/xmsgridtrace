echo off

echo.
echo OPTIONALLY DELETE BUILD FOLDER BEFORE MAKING IT NEXT: build_cpp
pause

echo on
mkdir build_py
pushd .
cd build_py
conan install -pr ..\dev\xmsprofile_py .. --build missing
echo off

echo.
echo ABORT IF ERRORS ABOVE
pause

echo on
cmake .. -G"Visual Studio 14 2015 Win64" -DIS_PYTHON_BUILD=True -DBUILD_TESTING=False -DPYTHON_TARGET_VERSION=3.5
popd
echo off

echo.
echo ABORT IF ERRORS ABOVE
pause

echo.
echo OPEN VS2015 FILE build_py\xmsgridtrace.sln
echo BUILD PROJECT xmsgridtrace_py FOR RELEASE x64
echo COPY PYTHON LIB .pyd FROM .conan AREA TO build_py\lib\.
echo RUN TEST FILE Test_py.bat
pause
