mkdir build
pushd .
cd build

conan install --build xmscore -pr ..\dev\xmsprofile_2015debug ..
pause
conan install --build xmsgrid -pr ..\dev\xmsprofile_2015debug ..
pause
conan install --build xmsinterp -pr ..\dev\xmsprofile_2015debug ..
pause
conan install -pr ..\dev\xmsprofile_2015debug ..
pause
cmake .. -G "Visual Studio 14 2015 Win64" -DXMS_BUILD=TRUE -DBUILD_TESTING=TRUE
pause
popd
