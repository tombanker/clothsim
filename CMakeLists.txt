cmake_minimum_required(VERSION 3.20)
project(clothsim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── GLFW ──────────────────────────────────────────────────────────────────────
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# ── GLM (header-only) ─────────────────────────────────────────────────────────
add_subdirectory(external/glm)

# ── Dear ImGui ────────────────────────────────────────────────────────────────
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

# ── GLAD ──────────────────────────────────────────────────────────────────────
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# ── Main executable ───────────────────────────────────────────────────────────
file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h)

add_executable(clothsim ${SRC_FILES})

target_include_directories(clothsim PRIVATE
    src/
    external/glm
    external/glad/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(clothsim PRIVATE
    glfw
    glad
    imgui
    glm
)

# macOS: link required system frameworks
if(APPLE)
    target_link_libraries(clothsim PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

# Copy shaders next to the executable so relative paths work
add_custom_command(TARGET clothsim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:clothsim>/shaders
    COMMENT "Copying shaders to build directory"
)