# roselex

Windows:

```
mkdir build && cd build
cmake ..
cmake --build . --config Release
ctest -C Release
Release\benchmarks.exe ..\data\words_alpha.txt
```

Linux:

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
ctest
./benchmarks ../data/words_alpha.txt
```
