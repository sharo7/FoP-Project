cmake_minimum_required(VERSION 4.1)
project(project)

set(CMAKE_CXX_STANDARD 17)

add_executable(project main.cpp
        sprite.h
        sprite.cpp
        motion_commands.h
        motion_commands.cpp)
target_link_libraries(${PROJECT_NAME} -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer)
