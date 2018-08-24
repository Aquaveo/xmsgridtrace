pip install numpy
pushd .
cd build_py\lib
python -m unittest discover -v -p *_pyt.py -s ../../xmsgridtrace/python/gridtrace
popd
pause

echo must copy libraries to lib folder:
echo xmscore_py.cp35-win_amd64.pyd
echo xmsextractor_py.cp35-win_amd64.pyd