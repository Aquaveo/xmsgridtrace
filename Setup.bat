mkdir build
pushd .
cd build
conan install -pr ..\dev\xmsprofile_release_py .. --build missing
pause
cmake .. -G "Visual Studio 14 2015 Win64" -DIS_PYTHON_BUILD=True 
pause
popd
