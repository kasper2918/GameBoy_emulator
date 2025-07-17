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

include_directories(ROMS)

add_executable (GameBoy_emu "GameBoy_emu/Main.cpp" "GameBoy_emu/Emulator/Emulator.cpp" "GameBoy_emu/Emulator/Emulator.h" "GameBoy_emu/Emulator/PublicFunctions.cpp" "GameBoy_emu/Emulator/Memory.cpp" "GameBoy_emu/Emulator/Misc/BitOps.h" "GameBoy_emu/Emulator/Timers.cpp" "GameBoy_emu/Emulator/Interrupts.cpp" "GameBoy_emu/Emulator/Misc/Utils.cpp" "GameBoy_emu/Emulator/LCD.cpp" "GameBoy_emu/Emulator/Misc/Misc.cpp" "GameBoy_emu/Emulator/Graphics.cpp" "GameBoy_emu/Emulator/Joypad.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET GameBoy_emu PROPERTY CXX_STANDARD 20)
endif()

add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

target_link_libraries(GameBoy_emu PRIVATE SDL3::SDL3)

add_definitions(MY_NGTEST)

if (EXISTS vendor/googletest-1.17.0)
	remove_definitions(MY_NGTEST)

	# For Windows: Prevent overriding the parent project's compiler/linker settings
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	add_subdirectory(vendor/googletest-1.17.0)
	enable_testing()
	include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
	include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

	add_executable(
	  hello_test
	  "GameBoy_emu/UnitTests.cpp"
	   "GameBoy_emu/Emulator/Emulator.cpp" "GameBoy_emu/Emulator/Emulator.h" "GameBoy_emu/Emulator/PublicFunctions.cpp" "GameBoy_emu/Emulator/Memory.cpp" "GameBoy_emu/Emulator/Misc/BitOps.h" "GameBoy_emu/Emulator/Timers.cpp" "GameBoy_emu/Emulator/Interrupts.cpp" "GameBoy_emu/Emulator/Misc/Utils.cpp" "GameBoy_emu/Emulator/LCD.cpp" "GameBoy_emu/Emulator/Misc/Misc.cpp" "GameBoy_emu/Emulator/Graphics.cpp" "GameBoy_emu/Emulator/Joypad.cpp")
	set_property(TARGET hello_test PROPERTY CXX_STANDARD 23)
	target_link_libraries(
	  hello_test
	  GTest::gtest_main
	  gmock_main
	)

	include(GoogleTest)
	gtest_discover_tests(hello_test)

	add_test(hello_test hello_test)
endif()