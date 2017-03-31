# coc
## building

You need ```boost/preprocessor``` in your system's include path.
Then, just do this:
```bash
mkdir coc
cd coc
git clone git@github.com:AnyDSL/thorin.git
git clone git@github.com:AnyDSL/coc.git
cd coc/thorin/util/
ln -s ../../../thorin/src/thorin/util/* . # yes, it's a hack...
cd ../..
mkdir build
cd build
cmake ..
make
```
