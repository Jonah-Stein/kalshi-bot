cmake_minimum_required(VERSION 3.20)
project(kalshi-bot LANGUAGES CXX) #Indicates using c++

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#for finding homebrew-installed packages
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(simdjson CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED)

add_executable(kalshi-bot
    main.cpp
    kalshi/KalshiAuth.cpp
    kalshi/KalshiRestClient.cpp
    kalshi/KalshiWsClient.cpp
    kalshi/KalshiMessageParser.cpp
    testscripts/testringbuffer.cpp
    testscripts/generatedeltas.cpp
    testscripts/testwebsocket.cpp
    testscripts/orderbookwithmessages.cpp
    testscripts/orderbookupdates.cpp
    orderbook/Orderbook.cpp
)

target_link_libraries(kalshi-bot PRIVATE
    Boost::headers
    OpenSSL::SSL
    OpenSSL::Crypto
    cpr::cpr
    simdjson::simdjson
)