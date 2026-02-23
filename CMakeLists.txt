cmake_minimum_required(VERSION 3.20)
project(Fop_Scratch)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)

add_executable(Fop_Scratch
        main.cpp
        sprite.cpp
        motion_commands.cpp
        looks_commands.cpp
        sound_commands.cpp
        events.cpp
        control_flow_logic.cpp
        sensing_commands.cpp
        operators.cpp
        variables.cpp
        interpreter.cpp
        sprite_editor.cpp
)

target_include_directories(Fop_Scratch PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
)

target_link_libraries(Fop_Scratch PRIVATE
        ${SDL2_LIBRARIES}
        SDL2_image
        SDL2_ttf
        SDL2_mixer
)


file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/saves)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/sprites)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/sounds)


add_executable(costume_editor
        costume_editor.cpp
        sprite_editor.cpp
)

target_include_directories(costume_editor PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
)

target_link_libraries(costume_editor PRIVATE
        ${SDL2_LIBRARIES}
        SDL2_image
        SDL2_ttf
        SDL2_mixer
)