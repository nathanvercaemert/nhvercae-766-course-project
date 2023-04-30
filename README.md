### Dependencies (Ubuntu 22.04)
build-essential, libclang-dev, clang

### Compilation
g++ ./i-o-analysis.cpp -o i-o-analysis -lclang -I /usr/lib/llvm-14/include/ -L /usr/lib/llvm-14/lib/

### Run Commands
- ./i-o-analysis ./convert.c
- ./i-o-analysis ./cpp_files.c

#### Run on All Benchmarks
- ./run-on-all-benchmarks.sh ./i-o-analysis ~/Desktop/benchspec/CPU/
- ./run-on-all-benchmarks.sh ./i-o-analysis (path to folder containing all benchmarks)

### Dockerfile
For use in the event that compilation or run fails. 
