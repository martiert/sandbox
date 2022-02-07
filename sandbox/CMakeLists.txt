project(sandbox)

add_library(sandbox SHARED sandbox.cpp)
target_link_libraries(sandbox cap seccomp dl)

install(TARGETS sandbox
    CONFIGURATIONS Release
    RUNTIME_DESTINATION lib)
