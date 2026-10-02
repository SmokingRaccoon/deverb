# Projeto separado do plugin (não entra no build do deVerb): só compila
# juce_dsp + dependências para a harness de testes offline.
cmake_minimum_required(VERSION 3.22)
project(deVerbTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../JUCE ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE/CMakeLists.txt)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
else()
  message(FATAL_ERROR "JUCE não encontrado para os testes.")
endif()

add_executable(test_dsp
  test_dsp.cpp
  ../Source/dsp/Delay.cpp
  ../Source/dsp/Gater.cpp
  ../Source/dsp/ReverbEngine.cpp
  ../Source/dsp/Granular.cpp
  ../Source/dsp/ReverseEngine.cpp
)

target_include_directories(test_dsp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(test_dsp PRIVATE
  juce::juce_dsp
  juce::juce_audio_basics
  juce::juce_audio_processors
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
)

target_compile_definitions(test_dsp PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
)

# Fonte embutida (partilhada com o plugin).
juce_add_binary_data(deVerbFontsTests
  HEADER_NAME DeVerbFonts.h
  NAMESPACE DeVerbFonts
  SOURCES
    ../Source/ui/fonts/NimbusSans-Regular.ttf
    ../Source/ui/fonts/NimbusSans-Bold.ttf
)

# Teste ponta-a-ponta: compila o processador+editor reais e corre áudio.
add_executable(test_chain
  test_chain.cpp
  ../Source/PluginProcessor.cpp
  ../Source/PluginEditor.cpp
  ../Source/ui/WaterLnF.cpp
  ../Source/ui/V4Viz.cpp
  ../Source/ui/V4ModulePanel.cpp
  ../Source/dsp/Delay.cpp
  ../Source/dsp/Gater.cpp
  ../Source/dsp/ReverbEngine.cpp
  ../Source/dsp/Granular.cpp
  ../Source/dsp/ReverseEngine.cpp
)

target_include_directories(test_chain PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(test_chain PRIVATE
  deVerbFontsTests
  juce::juce_audio_utils
  juce::juce_gui_basics
  juce::juce_dsp
  juce::juce_audio_processors
  juce::juce_audio_basics
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
)

target_compile_definitions(test_chain PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
)

# Harness de sessões heavy-user: conduz o editor real (precisa de display;
# correr sob xvfb-run). Não entra no build do plugin.
add_executable(test_ui
  test_ui.cpp
  ../Source/PluginProcessor.cpp
  ../Source/PluginEditor.cpp
  ../Source/ui/WaterLnF.cpp
  ../Source/ui/V4Viz.cpp
  ../Source/ui/V4ModulePanel.cpp
  ../Source/dsp/Delay.cpp
  ../Source/dsp/Gater.cpp
  ../Source/dsp/ReverbEngine.cpp
  ../Source/dsp/Granular.cpp
  ../Source/dsp/ReverseEngine.cpp
)

target_include_directories(test_ui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(test_ui PRIVATE
  deVerbFontsTests
  juce::juce_audio_utils
  juce::juce_gui_basics
  juce::juce_dsp
  juce::juce_audio_processors
  juce::juce_audio_basics
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
)

target_compile_definitions(test_ui PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_MODAL_LOOPS_PERMITTED=1
)
