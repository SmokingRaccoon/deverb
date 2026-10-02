cmake_minimum_required(VERSION 3.22)
project(deVerb VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ccache se disponível (poupa bué tempo neste i7)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Formatos: override no CI macOS com -DDEVERB_FORMATS="Standalone;VST3;AU".
set(DEVERB_FORMATS "Standalone;VST3;LV2" CACHE STRING "Formatos do plugin")

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt)
  # submodule (CI e checkouts frescos)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/JUCE ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt)
  # layout local antigo (../JUCE ao lado de deVerb/)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../JUCE ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
else()
  message(FATAL_ERROR "JUCE não encontrado: nem ./JUCE (submodule) nem ../JUCE existem.")
endif()

juce_add_plugin(deVerb
  COMPANY_NAME "PedroLabs"
  PLUGIN_MANUFACTURER_CODE Pdlb
  PLUGIN_CODE dVr1
  FORMATS ${DEVERB_FORMATS}
  PRODUCT_NAME "deVerb"
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_SYNTH FALSE
  NEEDS_WEB_BROWSER FALSE
  LV2URI https://pedrolabs.example/deverb
)

# Fonte embutida v4 (Nimbus Sans, decisão UI v4): via BinaryData.
juce_add_binary_data(deVerbFonts
  HEADER_NAME DeVerbFonts.h
  NAMESPACE DeVerbFonts
  SOURCES
    Source/ui/fonts/NimbusSans-Regular.ttf
    Source/ui/fonts/NimbusSans-Bold.ttf
)

target_sources(deVerb PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
  Source/core/TempoInfo.h
  Source/dsp/Delay.h
  Source/dsp/Delay.cpp
  Source/dsp/Gater.h
  Source/dsp/Gater.cpp
  Source/dsp/ReverbEngine.h
  Source/dsp/ReverbEngine.cpp
  Source/dsp/Granular.h
  Source/dsp/Granular.cpp
  Source/dsp/ReverseEngine.h
  Source/dsp/ReverseEngine.cpp
  Source/core/RevLinker.h
  Source/ui/BeatRuler.h
  Source/ui/AbletonLnF.h
  Source/ui/WaterLnF.h
  Source/ui/WaterLnF.cpp
  Source/ui/V4Dial.h
  Source/ui/V4Seg.h
  Source/ui/V4Stepper.h
  Source/ui/V4Num.h
  Source/ui/V4Key.h
  Source/ui/V4Viz.h
  Source/ui/V4Viz.cpp
  Source/ui/V4ModulePanel.h
  Source/ui/V4ModulePanel.cpp
  Source/ui/V4Tile.h
)

target_link_libraries(deVerb PRIVATE
  deVerbFonts
  juce::juce_audio_utils
  juce::juce_dsp
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
)

target_compile_definitions(deVerb PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)
