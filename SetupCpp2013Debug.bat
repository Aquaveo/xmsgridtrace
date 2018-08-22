mkdir build
pushd .
cd build
conan install -pr ..\dev\xmsprofile_2013debug ..
pause
conan install --build xmscore -pr ..\dev\xmsprofile_2013debug ..
conan install --build xmsgrid -pr ..\dev\xmsprofile_2013debug ..
conan install --build xmsinterp -pr ..\dev\xmsprofile_2013debug ..
pause
cmake .. -G "Visual Studio 12 2013 Win64" -DXMS_BUILD=TRUE -DBUILD_TESTING=TRUE
pause
popd
