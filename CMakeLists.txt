cmake_minimum_required(VERSION 3.16)

project(XSIM LANGUAGES CXX)

option(XSIM_STATIC_RUNTIME "Try to link the CLI with a static C/C++ runtime where supported" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(XSIM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DJSSP_PSO_HH")

add_library(xsim_core STATIC
    ${XSIM_SOURCE_DIR}/BatchConfigLoader.cpp
    ${XSIM_SOURCE_DIR}/BatchRunner.cpp
    ${XSIM_SOURCE_DIR}/ConfigLoader.cpp
    ${XSIM_SOURCE_DIR}/EventScheduleGenerator.cpp
    ${XSIM_SOURCE_DIR}/FeasibilityChecker.cpp
    ${XSIM_SOURCE_DIR}/FeatureVectorBuilder.cpp
    ${XSIM_SOURCE_DIR}/GtScheduleGenerator.cpp
    ${XSIM_SOURCE_DIR}/InstanceCatalog.cpp
    ${XSIM_SOURCE_DIR}/InstanceGenerator.cpp
    ${XSIM_SOURCE_DIR}/InstanceLoader.cpp
    ${XSIM_SOURCE_DIR}/Pso.cpp
    ${XSIM_SOURCE_DIR}/ResultWriter.cpp
    ${XSIM_SOURCE_DIR}/RuleRegistry.cpp
    ${XSIM_SOURCE_DIR}/Simulation.cpp
)

target_include_directories(xsim_core
    PUBLIC
        ${XSIM_SOURCE_DIR}
)

target_compile_features(xsim_core PUBLIC cxx_std_17)

add_executable(xsim_cli
    ${XSIM_SOURCE_DIR}/main.cpp
)

target_link_libraries(xsim_cli PRIVATE xsim_core)
target_compile_features(xsim_cli PRIVATE cxx_std_17)

set_target_properties(xsim_cli PROPERTIES
    OUTPUT_NAME "djssp_pso_hh"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
)

if (XSIM_STATIC_RUNTIME)
    if (MSVC)
        set_property(TARGET xsim_core PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET xsim_cli PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(xsim_cli PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()

enable_testing()

find_program(POWERSHELL_EXECUTABLE NAMES pwsh powershell)
if (POWERSHELL_EXECUTABLE)
    add_test(
        NAME xsim_smoke_positive
        COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass
                -File ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_smoke.ps1
                -ExePath $<TARGET_FILE:xsim_cli>
    )
    set_tests_properties(xsim_smoke_positive PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_test(
        NAME xsim_smoke_negative
        COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass
                -File ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_negative_smoke.ps1
                -ExePath $<TARGET_FILE:xsim_cli>
    )
    set_tests_properties(xsim_smoke_negative PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
