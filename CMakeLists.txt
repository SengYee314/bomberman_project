cmake_minimum_required(VERSION 3.16)
project(BomberLab C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
option(HEADLESS_ONLY "Build only pure-C logic tests (no raylib)" OFF)
option(STUDENT_BUILD "Label GUI as student skeleton" ON)

# ---- 사전 점검: 흔한 실패를 raylib 빌드 전에 알아보기 쉬운 메시지로 막는다. ----
# (Windows 콘솔 인코딩에 상관없이 읽히도록 메시지는 영어로 쓴다.)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "Do not build inside the project folder.\n"
    "Delete CMakeCache.txt and CMakeFiles/ here, then run:  cmake -S . -B build")
endif()
string(REGEX MATCH "[^ -~]" non_ascii_path "${CMAKE_SOURCE_DIR}${CMAKE_BINARY_DIR}")
if(non_ascii_path)
  message(WARNING "The project path contains non-ASCII characters (e.g. Korean folder names).\n"
    "Some compilers fail on such paths. If the build fails, move the folder to e.g. C:/bomber or ~/bomber.")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/spritesheet.png")
  message(FATAL_ERROR "assets/spritesheet.png is missing. Unzip the whole project, not only src/.")
endif()

add_executable(test_game tests/test_game.c src/game.c src/rules.c)
target_include_directories(test_game PRIVATE src)
if(MSVC)
  target_compile_options(test_game PRIVATE /std:c11 /utf-8)
endif()
if(NOT MSVC)
  target_link_libraries(test_game PRIVATE m)
  target_compile_options(test_game PRIVATE -Wall -Wextra)
endif()
# 공개 예제: IDE 실행 목록에서 바로 돌릴 수 있게 타깃으로 둔다(터미널·PATH 설정 불필요).
add_executable(test_rules_public tests/test_rules_public.c src/rules.c)
target_include_directories(test_rules_public PRIVATE src)
if(MSVC)
  target_compile_options(test_rules_public PRIVATE /std:c11 /utf-8)
else()
  target_link_libraries(test_rules_public PRIVATE m)
endif()
enable_testing()
add_test(NAME core_rules COMMAND test_game)
add_test(NAME public_examples COMMAND test_rules_public)

add_executable(test_stage_start tests/test_stage_start.c src/rules.c)
target_include_directories(test_stage_start PRIVATE src)
if(MSVC)
  target_compile_options(test_stage_start PRIVATE /std:c11 /utf-8)
else()
  target_link_libraries(test_stage_start PRIVATE m)
  target_compile_options(test_stage_start PRIVATE -Wall -Wextra)
endif()
add_test(NAME stage_start COMMAND test_stage_start)

# 제출 전 점검: 채점기처럼 src/rules.c만 배포 당시 원본 헤더·엔진(tests/contract)과 빌드한다.
# rules.c를 빌드 폴더로 복사해야 #include "rules.h"가 src/가 아닌 원본 헤더를 찾는다.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/contract/rules.h")
  set(CONTRACT_RULES "${CMAKE_CURRENT_BINARY_DIR}/contract/rules.c")
  add_custom_command(OUTPUT "${CONTRACT_RULES}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/src/rules.c" "${CONTRACT_RULES}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/rules.c")
  add_executable(check_submission "${CONTRACT_RULES}" tests/contract/game.c tests/test_rules_public.c)
  target_include_directories(check_submission PRIVATE tests/contract)
  target_compile_definitions(check_submission PRIVATE SUBMISSION_CHECK=1)
  if(MSVC)
    target_compile_options(check_submission PRIVATE /std:c11 /utf-8)
  else()
    target_link_libraries(check_submission PRIVATE m)
  endif()
endif()

if(NOT HEADLESS_ONLY)
  # Linux는 X11·OpenGL 개발 패키지가 없으면 raylib(GLFW) 빌드 중간에 알아보기 어려운 오류가 난다.
  if(UNIX AND NOT APPLE)
    find_package(X11 QUIET)
    find_package(OpenGL QUIET)
    if(NOT X11_FOUND OR NOT X11_Xrandr_FOUND OR NOT X11_Xinerama_FOUND OR NOT X11_Xcursor_FOUND OR NOT X11_Xi_FOUND OR NOT OPENGL_FOUND)
      message(FATAL_ERROR "X11/OpenGL development packages are missing. On Ubuntu/Debian run:\n"
        "  sudo apt install build-essential cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libxcursor-dev libxinerama-dev\n"
        "Then delete the build folder and configure again.")
    endif()
  endif()

  # raylib 5.5 소스를 프로젝트에 함께 넣어 두어 인터넷 다운로드 없이 빌드한다(네트워크·인증서·프록시 오류 차단).
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(RAYLIB_BUNDLED "${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib-5.5")
  if(EXISTS "${RAYLIB_BUNDLED}/CMakeLists.txt")
    add_subdirectory("${RAYLIB_BUNDLED}" raylib-build EXCLUDE_FROM_ALL)
  else()
    find_package(raylib 5.5 QUIET)
    if(NOT raylib_FOUND)
      message(STATUS "third_party/raylib-5.5 not found: downloading raylib 5.5 (internet required)")
      include(FetchContent)
      # 일부 Windows용 CMake(WinLibs/MinGW 등)는 CA 인증서 묶음을 찾지 못해 GitHub HTTPS 다운로드가
      # "SSL peer certificate ... was not OK"로 실패한다. 지정된 것이 없으면 설치된 묶음을 사용한다.
      if(WIN32 AND NOT CMAKE_TLS_CAINFO)
        foreach(ca_file "$ENV{SSL_CERT_FILE}" "$ENV{ProgramFiles}/Git/mingw64/etc/ssl/certs/ca-bundle.crt" "$ENV{ProgramFiles}/Git/usr/ssl/certs/ca-bundle.crt")
          file(TO_CMAKE_PATH "${ca_file}" ca_file)  # 역슬래시는 다운로드 스크립트를 깨뜨린다
          if(ca_file AND EXISTS "${ca_file}")
            set(CMAKE_TLS_CAINFO "${ca_file}")
            break()
          endif()
        endforeach()
      endif()
      FetchContent_Declare(raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
        URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e)
      FetchContent_MakeAvailable(raylib)
    endif()
  endif()
  add_executable(bomber_lab src/main.c src/game.c src/rules.c src/renderer.c)
  target_link_libraries(bomber_lab PRIVATE raylib)
  if(NOT MSVC)
    target_link_libraries(bomber_lab PRIVATE m)
  else()
    target_compile_options(bomber_lab PRIVATE /std:c11 /utf-8)
  endif()
  if(STUDENT_BUILD)
    target_compile_definitions(bomber_lab PRIVATE STUDENT_BUILD=1)
  endif()
  add_custom_command(TARGET bomber_lab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:bomber_lab>/assets)
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT bomber_lab)
  set_property(TARGET bomber_lab PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:bomber_lab>")
endif()
