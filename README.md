# 2d-artistic-animation
2D artistic animation using openGL
# Requirements

- OS: Windows 10/11

- GPU/Driver: OpenGL 2.1+ (or 3.3 Compatibility profile)

- Compiler: Visual Studio 2019/2022 (MSVC, C++17) or MinGW-w64 (g++)

- Libraries (link time):

opengl32.lib (OpenGL)

glfw3.lib (or glfw3dll.lib if using the DLL)

winmm.lib (for MP3 playback via MCI)

user32.lib, gdi32.lib, shell32.lib

- Headers:

<Windows.h> (must be included before <GL/gl.h>)

<GLFW/glfw3.h>, <GL/gl.h>

No GLAD required (immediate-mode OpenGL only).

- Assets: Place Take Five.mp3 next to the executable (used for sync/playback).
