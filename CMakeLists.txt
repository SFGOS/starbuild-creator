cmake_minimum_required(VERSION 3.16)
project(StarbuildCreator VERSION 1.0 LANGUAGES C)

# Require C99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Add CLI executable
add_executable(${PROJECT_NAME} src/main.c)

# Install targets
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)