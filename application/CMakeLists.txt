project(application)

add_executable(application application.cpp)
target_link_libraries(application cap)

install(TARGETS application
    CONFIGURATIONS Release
    RUNTIME_DESTINATION bin)
