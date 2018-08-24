echo off

echo.
echo Copy libraries to build_py/lib folder:
echo xmsgrid_py.cp35-win_amd64.pyd
echo xmsextractor_py.cp35-win_amd64.pyd
pause

echo on
pip install numpy
pushd .
cd build_py\lib
python -m unittest discover -v -p *_pyt.py -s ../../xmsgridtrace/python/gridtrace
popd
echo off

echo.
echo Look for tests ran OK
pause

