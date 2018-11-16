cmake_minimum_required(VERSION 3.12)
project(webServer)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)
include_directories(src/cache_tests)

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

add_executable(webServer
        src/cache.c
        src/cache.h
        src/file.c
        src/file.h
        src/hashtable.c
        src/hashtable.h
        src/llist.c
        src/llist.h
        src/mime.c
        src/mime.h
        src/net.c
        src/net.h
        src/server.c)

target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto)

