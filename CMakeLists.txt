cmake_minimum_required(VERSION 4.0)
project(Scratch)

set(CMAKE_CXX_STANDARD 26)

add_executable(Scratch main.cpp
        sprite.h
        sprite.cpp
        motion_commands.h
        motion_commands.cpp
        looks_commands.h
        looks_commands.cpp
        sound_commands.h
        sound_commands.cpp
        control_flow_logic.cpp
        control_flow_logic.h
        sensing_commands.h
        sensing_commands.cpp
        operators.h
        operators.cpp
        variables.h
        variables.cpp
        interpreter.h
        interpreter.cpp)

target_link_libraries(${PROJECT_NAME} -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer)
