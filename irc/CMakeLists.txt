cmake_minimum_required(VERSION 3.0)
project(irc_client)

# Add the executable
add_executable(irc_client main.cpp)

# Link against OpenSSL libraries
find_package(OpenSSL REQUIRED)
target_link_libraries(irc_client PRIVATE OpenSSL::SSL OpenSSL::Crypto)