```cmake
FetchContent_Declare(
    eventsystem
    GIT_REPOSITORY https://github.com/NikDex/EventSystem.git
    GIT_TAG master
)
FetchContent_MakeAvailable(eventsystem)
FetchContent_GetProperties(eventsystem)
include_directories(${eventsystem_SOURCE_DIR}/include)
```