# coc
## building

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
