g++ -I ../src/iibmalloc/src/foundation/include -I ../src/iibmalloc/src/foundation/3rdparty/fmt/include -shared -o libgcc_lto_workaround.so -fPIC gcc_lto_workaround.cpp