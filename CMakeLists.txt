# CMakeList.txt: файл проекта CMake верхнего уровня; проведите глобальную конфигурацию
# и включите сюда подпроекты.
#
cmake_minimum_required (VERSION 3.8)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("CMakeProject1")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Включите подпроекты.
add_subdirectory ("vendor")
add_subdirectory ("GameBoy_emu")

add_executable (GameBoy_emu "GameBoy_emu/CMakeProject1.cpp" "GameBoy_emu/CMakeProject1.h" "GameBoy_emu/Types.h" "GameBoy_emu/CPU/Emulator.h" "GameBoy_emu/CPU/Emulator.cpp" "GameBoy_emu/CPU/PublicFunctions.cpp" "GameBoy_emu/CPU/Memory.cpp" "GameBoy_emu/CPU/Timers.cpp" "GameBoy_emu/CPU/Interupts.cpp" "GameBoy_emu/CPU/LCD.cpp" "GameBoy_emu/CPU/Misc.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET GameBoy_emu PROPERTY CXX_STANDARD 20)
endif()

add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

target_link_libraries(GameBoy_emu PRIVATE SDL3::SDL3)
