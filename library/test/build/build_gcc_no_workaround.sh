g++ ../test_safe_pointers.cpp ../../../library/src/safe_ptr.cpp ../../../../library/src/nodecpp_error.cpp ../../../library/gcc_lto_workaround/gcc_lto_workaround.cpp ../../../library/src/iibmalloc/src/foundation/src/std_error.cpp ../../../library/src/iibmalloc/src/foundation/src/safe_memory_error.cpp ../../../library/src/iibmalloc/src/foundation/src/log.cpp ../../../library/src/iibmalloc/src/foundation/src/tagged_ptr_impl.cpp ../../../library/src/iibmalloc/src/foundation/3rdparty/fmt/src/format.cc ../../../library/src/iibmalloc/src/foundation/src/nodecpp_assert.cpp ../../../library/src/iibmalloc/src/foundation/src/page_allocator.cpp ../../../library/src/iibmalloc/src/iibmalloc.cpp -I../../../library/src/iibmalloc/src -I../../../library/src -I../../../library/include -I../../../library/src/iibmalloc/src/foundation/include -I../../../library/src/iibmalloc/src/foundation/3rdparty/fmt/include -std=c++17 -DUSING_T_SOCKETS -g -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-empty-body -DNDEBUG -O2 -flto -lpthread -o test.bin