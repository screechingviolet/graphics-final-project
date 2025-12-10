cmake_minimum_required(VERSION 3.16)

# Sets project name
project(projects_realtime LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Sets C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specifies required Qt components
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt6 REQUIRED COMPONENTS OpenGL)
find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)
find_package(Qt6 REQUIRED COMPONENTS Xml)

# Allows you to include files from within those directories, without prefixing their filepaths
include_directories(src)

# Specifies .cpp and .h files to be passed to the compiler
add_executable(${PROJECT_NAME}
    src/main.cpp

    src/realtime.cpp
    src/mainwindow.cpp
    src/settings.cpp
    src/utils/scenefilereader.cpp
    src/utils/sceneparser.cpp

    src/mainwindow.h
    src/realtime.h
    src/settings.h
    src/utils/scenedata.h
    src/utils/scenefilereader.h
    src/utils/sceneparser.h
    src/utils/shaderloader.h
    src/utils/aspectratiowidget/aspectratiowidget.hpp
    src/shapes/Cone.cpp src/shapes/Cone.h src/shapes/Cube.cpp src/shapes/Cube.h src/shapes/Cylinder.cpp src/shapes/Cylinder.h src/shapes/Sphere.cpp src/shapes/Sphere.h
    src/camera.h src/camera.cpp
    src/cgltf.h
    src/shapes/mesh.h src/shapes/mesh.cpp
    src/uniforms.cpp
    src/geometry.cpp
    src/postprocessing/postprocess.h src/postprocessing/postprocess.cpp
    src/postprocessing/colorgrade.h src/postprocessing/colorgrade.cpp
    src/postprocessing/convolution.h src/postprocessing/convolution.cpp
    src/camerapaths/camerapath.h src/camerapaths/camerapath.cpp

    src/utils/miscutilities.h
    src/utils/miscutilities.cpp
    src/utils/particles.cpp
    src/utils/lsystems.cpp
    src/particles.h
    src/postprocessing/fog.cpp
    src/postprocessing/fog.h
    src/wavefunctioncollapse.h src/wavefunctioncollapse.cpp
    src/pngio.h src/pngio.cpp




)

# GLM: this creates its library and allows you to `#include "glm/..."`
add_subdirectory(glm)

# GLEW: this creates its library and allows you to `#include "GL/glew.h"`
add_library(StaticGLEW STATIC glew/src/glew.c)
include_directories(${PROJECT_NAME} PRIVATE glew/include)

# Specifies libraries to be linked (Qt components, glew, etc)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::OpenGLWidgets
    Qt::Xml
    StaticGLEW
)

# Specifies other files
qt6_add_resources(${PROJECT_NAME} "Resources"
    PREFIX
        "/"
    FILES
        resources/shaders/default.frag
        resources/shaders/default.vert
        resources/shaders/anim.frag
        resources/shaders/anim.vert
        resources/shaders/skybox.frag
        resources/shaders/skybox.vert
        resources/shaders/colorgrading.frag
        resources/shaders/texture.vert
	resources/shaders/particles.vert
	resources/shaders/particles.frag
	resources/shaders/fog.vert
	resources/shaders/fog.frag
	resources/shaders/scrolling.frag

        resources/images/test_lut_2.png
        resources/images/test_lut_2_8bit.png
        resources/images/vintage_warmth.png
        resources/images/autocontrast.png
        resources/images/color_swap.png
        resources/images/color_swap_2.png
        resources/images/dark.png
        resources/images/doom.png
        resources/images/doom_old.png
        resources/images/grayscale.png
        resources/images/grayscale_no_sat.png
        resources/images/greeny.png
        resources/images/green_neon.png
        resources/images/inverted.png
        resources/images/sepia.png
        resources/images/identity.png
        resources/images/Cold_Ice.png
        resources/images/park.png

)

# GLEW: this provides support for Windows (including 64-bit)
if (WIN32)
  add_compile_definitions(GLEW_STATIC)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    opengl32
    glu32
  )
endif()

# Set this flag to silence warnings on Windows
if (MSVC OR MSYS OR MINGW)
  set(CMAKE_CXX_FLAGS "-Wno-volatile")
endif()
# Set this flag to silence warnings on MacOS
if (APPLE)
  set(CMAKE_CXX_FLAGS "-Wno-deprecated-volatile")
endif()
