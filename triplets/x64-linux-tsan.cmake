# x64-linux with every dependency built under ThreadSanitizer. TSan only understands synchronisation in
# code it instrumented, so uninstrumented libraries, such as Drogon's lock-free queues, would show up as
# false races.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_C_FLAGS "-fsanitize=thread")
set(VCPKG_CXX_FLAGS "-fsanitize=thread")
set(VCPKG_LINKER_FLAGS "-fsanitize=thread")
