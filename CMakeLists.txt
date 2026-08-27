cmake_minimum_required(VERSION 3.10)
project(Forge)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(framework)

add_library(ForgeLib STATIC
    framework/Response.cpp
    framework/ThreadPool.cpp
    framework/Middleware.cpp
    framework/StaticFiles.cpp
    framework/Router.cpp
    framework/Session.cpp
    framework/Logger.cpp
    framework/Multipart.cpp
    framework/Template.cpp
    framework/sqlite3.c
    framework/Database.cpp
    framework/RequestHandler.cpp
    framework/Server.cpp
    framework/Auth.cpp
    framework/Config.cpp
    framework/Request.cpp
    framework/RateLimiter.cpp
)
target_link_libraries(ForgeLib Ws2_32 Bcrypt)

add_executable(Forge main.cpp)
target_link_libraries(Forge ForgeLib)

add_executable(ForgeTests tests/test_main.cpp)
target_link_libraries(ForgeTests ForgeLib)
