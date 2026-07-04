cmake_minimum_required(VERSION 4.2)
project(CPPAnimProgramming)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCE_FILES
        main.cpp
        Tools/*.cpp
        Window/*.cpp
        Render/*.cpp
        Model/*.cpp)

find_package(OpenGL)
find_package(glfw3)
find_package(glm)
find_package(glad)
find_package(stb)
find_package(imgui)
find_package(TinyGLTF)

#get_filename_component(IMGUI_PACKAGE_ROOT "${imgui_INCLUDE_DIRS}" DIRECTORY)
#set(IMGUI_BINDINGS_DIR "${IMGUI_PACKAGE_ROOT}/res/bindings")
set(IMGUI_BINDINGS_DIR "${imgui_INCLUDE_DIRS}/../res/bindings")
message("ImGui bindings directory: ${IMGUI_BINDINGS_DIR}")

set(IMGUI_BINDINGS_SOURCES
        "${IMGUI_BINDINGS_DIR}/imgui_impl_glfw.cpp"
        "${IMGUI_BINDINGS_DIR}/imgui_impl_opengl3.cpp"
)

add_executable(CPPAnimProgramming ${SOURCE_FILES} ${IMGUI_BINDINGS_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_BINDINGS_DIR})

# copy shader files
add_custom_command(
        TARGET CPPAnimProgramming POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
        "$<TARGET_FILE_DIR:CPPAnimProgramming>/Shaders"
)

# copy textures
add_custom_command(
        TARGET CPPAnimProgramming POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Textures"
        "$<TARGET_FILE_DIR:CPPAnimProgramming>/Resources/Textures"
)

# copy assets
add_custom_command(
        TARGET CPPAnimProgramming POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Assets"
        "$<TARGET_FILE_DIR:CPPAnimProgramming>/Resources/Assets"
)

target_link_libraries(CPPAnimProgramming
        OpenGL::GL
        glfw
        glm::glm
        glad::glad
        stb::stb
        imgui::imgui
        TinyGLTF::TinyGLTF)