cmake_minimum_required (VERSION 3.30.5)

option(EVENTSYSTEM_ENABLE_TESTING "Enable testing" ON)
option(EVENTSYSTEM_FETCH_ENTT "Fetch EnTT" ON)
option(EVENTSYSTEM_FETCH_GOOGLETEST "Fetch GoogleTest" ON)

set(FETCH_ENABLED EVENTSYSTEM_FETCH_ENTT OR EVENTSYSTEM_FETCH_GOOGLETEST)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14.2.0)
        message(ERROR "use GNU >= version 14.2.0")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17.0.1)
        message(ERROR "use Clang >= version 17.0.0")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.37)
        message(ERROR "use MSVC cl.exe >= 19.37.xxxxx (>= Visual Studio 2022 17.7)")
endif()

project(EventSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(EventSystem INTERFACE)
target_include_directories(EventSystem INTERFACE single_include)

if(FETCH_ENABLED)
    include(FetchContent)
endif()

if(EVENTSYSTEM_FETCH_ENTT)
    FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG        v3.15.0
    )
    FetchContent_MakeAvailable(entt)
endif()

if(EVENTSYSTEM_ENABLE_TESTING)
    if(EVENTSYSTEM_FETCH_GOOGLETEST)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.17.0
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    
    enable_testing()
    add_subdirectory(tests)
endif()