raylib 5.5 (https://github.com/raysan5/raylib, tag 5.5) source, bundled so that the game builds
without downloading anything. Only the files raylib's CMake build needs are included:
CMakeLists.txt, CMakeOptions.txt, raylib.pc.in, README.md, cmake/, src/ and LICENSE.
raylib is distributed under the zlib/libpng license (see raylib-5.5/LICENSE).
One local patch: src/rcore.c InitWindow() returns early when InitPlatform() fails
(no display / no OpenGL 3.3) instead of crashing, so the game can print a clear message.
Do not modify these files.
