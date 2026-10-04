cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(VGUI2Extension LANGUAGES C CXX RC)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "VGUI2Extension requires Windows MSVC x86. Configure with -A Win32.")
endif()
get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(multi_config)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
    message(FATAL_ERROR "Select Debug or Release.")
endif()

include(cmake/Dependencies.cmake)
vgui2extension_prepare_dependencies()
include("${VC_LTL_Root}/VC-LTL helper for cmake.cmake")
include(cmake/Sources.cmake)
add_library(VGUI2Extension SHARED ${VGUI2EXTENSION_SOURCES})
target_compile_features(VGUI2Extension PRIVATE cxx_std_20)
target_include_directories(VGUI2Extension PRIVATE
    src include/Interface include/Interface/VGUI
    "${METAHOOK_SOURCE_PATH}/include"
    "${METAHOOK_SOURCE_PATH}/include/Interface"
    "${METAHOOK_SOURCE_PATH}/include/Interface/VGUI"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/cl_dll"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/common"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/engine"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/pm_shared"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/public"
    "${METAHOOK_SOURCE_PATH}/include/VGUI"
    "${METAHOOK_SOURCE_PATH}/include/vgui_controls"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK/tier0"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK/tier1"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK/vstdlib"
    ${VGUI2EXTENSION_SDL3_INCLUDE_DIRS}
    ${VGUI2EXTENSION_SDL2_INCLUDE_DIRS}
)
target_compile_definitions(VGUI2Extension PRIVATE WIN32 _WINDOWS _USRDLL _MBCS
    STEAM_API_NODLL VGUI_USE_SURFACE2 VGUI_USE_SCHEME2 VGUI_USE_INPUT2
    NO_MALLOC_OVERRIDE NO_TIER0_MINIDUMP NO_VCR $<$<CONFIG:Debug>:_DEBUG>)
target_compile_options(VGUI2Extension PRIVATE /permissive /W3 /MP /Zi
    /wd4291 /wd4311 /wd4312 /wd4819 /wd4996 $<$<CONFIG:Release>:/wd4091>)
target_link_libraries(VGUI2Extension PRIVATE imm32)
target_link_options(VGUI2Extension PRIVATE /DEBUG /SUBSYSTEM:WINDOWS
    $<$<CONFIG:Debug>:/INCREMENTAL>
    $<$<CONFIG:Release>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF>)
set_target_properties(VGUI2Extension PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

install(TARGETS VGUI2Extension RUNTIME DESTINATION svencoop/metahook/plugins)
install(FILES "$<TARGET_PDB_FILE:VGUI2Extension>" DESTINATION svencoop/metahook/plugins)
install(DIRECTORY assets/ DESTINATION .)
install(DIRECTORY include/ DESTINATION include)

option(VGUI2EXTENSION_SYNC_GAMEDATA "Download and prune the plugin gamedata catalog before building" ON)
set(VGUI2EXTENSION_GAMEDATA_DIR "${PROJECT_BINARY_DIR}/assets/svencoop/metahook/gamedata/vgui2extension" CACHE PATH "Plugin gamedata catalog directory")
if(VGUI2EXTENSION_SYNC_GAMEDATA)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_custom_target(VGUI2ExtensionGameData
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/sync-gamedata.py"
            --manifest "${CMAKE_CURRENT_SOURCE_DIR}/scripts/manifests/vgui2extension.json"
            --target-dir "${VGUI2EXTENSION_GAMEDATA_DIR}" --temp-root "${PROJECT_BINARY_DIR}/gamedata-sync"
        COMMENT "Synchronizing and validating VGUI2Extension gamedata" VERBATIM)
    add_custom_target(VGUI2ExtensionGameDataValidate
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/validate-gamedata.py"
            "${VGUI2EXTENSION_GAMEDATA_DIR}" --manifest "${CMAKE_CURRENT_SOURCE_DIR}/scripts/manifests/vgui2extension.json"
        COMMENT "Validating VGUI2Extension gamedata against the manifest" VERBATIM)
    add_dependencies(VGUI2ExtensionGameDataValidate VGUI2ExtensionGameData)
    add_dependencies(VGUI2Extension VGUI2ExtensionGameDataValidate)
endif()
install(DIRECTORY "${VGUI2EXTENSION_GAMEDATA_DIR}/" DESTINATION svencoop/metahook/gamedata/vgui2extension OPTIONAL)

option(VGUI2EXTENSION_BUILD_TESTS "Build existing language and IME regression tests" OFF)
if(VGUI2EXTENSION_BUILD_TESTS)
    enable_testing()
    foreach(test language_registry_tests ime_window_message_test)
        add_executable(${test} "tests/${test}.cpp")
        target_compile_features(${test} PRIVATE cxx_std_17)
        target_compile_options(${test} PRIVATE /W4 /WX /UNDEBUG)
        add_test(NAME ${test} COMMAND ${test})
    endforeach()
endif()
