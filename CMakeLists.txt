cmake_minimum_required(VERSION 3.16)
project(tts LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Position Independent Code for all targets (required for shared libs/Python bindings)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 可执行文件输出到 build/bin 目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# =============================================================================
# 第三方依赖 - 自动克隆到 ~/.cache
# =============================================================================

# 获取用户 cache 目录
if(DEFINED ENV{HOME})
    set(USER_CACHE_DIR "$ENV{HOME}/.cache")
else()
    set(USER_CACHE_DIR "${CMAKE_BINARY_DIR}/.cache")
endif()

# cppjieba (中文分词) - 自动克隆到 ~/.cache (需要 --recursive 克隆子模块)
set(CPPJIEBA_DIR "${USER_CACHE_DIR}/cppjieba")
if(NOT EXISTS ${CPPJIEBA_DIR}/include/cppjieba/Jieba.hpp)
    message(STATUS "Cloning cppjieba to ${CPPJIEBA_DIR}...")
    execute_process(
        COMMAND git clone --recursive https://github.com/yanyiwu/cppjieba.git ${CPPJIEBA_DIR}
        RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone cppjieba")
    endif()
endif()

# cpp-pinyin (拼音转换) - 自动克隆到 ~/.cache
set(CPP_PINYIN_DIR "${USER_CACHE_DIR}/cpp-pinyin")
if(NOT EXISTS ${CPP_PINYIN_DIR}/include/cpp-pinyin/Pinyin.h)
    message(STATUS "Cloning cpp-pinyin to ${CPP_PINYIN_DIR}...")
    execute_process(
        COMMAND git clone https://github.com/wolfgitpr/cpp-pinyin.git ${CPP_PINYIN_DIR}
        RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone cpp-pinyin")
    endif()
endif()

# =============================================================================
# TTS Library
# =============================================================================

# 源文件
set(TTS_SOURCES
    src/tts_engine.cpp
    src/tts_backend_factory.cpp
    src/audio/audio_processor.cpp
    src/text/text_utils.cpp
    src/text/number_utils.cpp
    src/text/text_normalizer.cpp
    src/text/token_utils.cpp
    src/text/phoneme_utils.cpp
    src/vocoder/vocoder.cpp
    src/backends/matcha/matcha_backend.cpp
    src/backends/matcha/matcha_zh_backend.cpp
    src/backends/matcha/matcha_en_backend.cpp
    src/backends/matcha/matcha_zh_en_backend.cpp
    src/backends/matcha/tts_model_downloader.cpp
    src/backends/kokoro/kokoro_backend.cpp
    src/backends/kokoro/kokoro_phonemizer.cpp
    src/backends/kokoro/kokoro_voice_manager.cpp
    src/backends/kokoro/kokoro_model_downloader.cpp
)

# cpp-pinyin 源文件
set(CPP_PINYIN_SRC_DIR ${CPP_PINYIN_DIR}/src)
list(APPEND TTS_SOURCES
    ${CPP_PINYIN_SRC_DIR}/ChineseG2p.cpp
    ${CPP_PINYIN_SRC_DIR}/DictUtil.cpp
    ${CPP_PINYIN_SRC_DIR}/G2pglobal.cpp
    ${CPP_PINYIN_SRC_DIR}/Pinyin.cpp
    ${CPP_PINYIN_SRC_DIR}/PinyinRes.cpp
    ${CPP_PINYIN_SRC_DIR}/ToneConverter.cpp
    ${CPP_PINYIN_SRC_DIR}/U16Str.cpp
    ${CPP_PINYIN_SRC_DIR}/toneUtil/ManTone.cpp
    ${CPP_PINYIN_SRC_DIR}/toneUtil/ManToneUtil.cpp
)

# 创建静态库
add_library(tts STATIC ${TTS_SOURCES})

# Include 目录
target_include_directories(tts
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# =============================================================================
# 依赖项
# =============================================================================

# ONNX Runtime
# 用户可通过以下变量自定义路径:
#   -DONNXRUNTIME_INCLUDE_DIR=/path/to/include
#   -DONNXRUNTIME_LIB="/path/to/libonnxruntime.so;/path/to/other.so"
#
# 示例 (RISC-V):
#   cmake -B build -S . \
#     -DONNXRUNTIME_INCLUDE_DIR=$HOME/codes/spacemit-ort.riscv64.2.0.2+beta0/include \
#     -DONNXRUNTIME_LIB="$HOME/codes/spacemit-ort.riscv64.2.0.2+beta0/lib/libonnxruntime.so;$HOME/muggle/lib/libspacemit_ep.so"

set(ONNXRUNTIME_INCLUDE_DIR "" CACHE PATH "ONNX Runtime include directory")
set(ONNXRUNTIME_LIB "" CACHE STRING "ONNX Runtime libraries (semicolon-separated)")

if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIB)
    # 使用用户指定的路径
    message(STATUS "[tts] Using user-specified ONNX Runtime:")
    message(STATUS "  Include: ${ONNXRUNTIME_INCLUDE_DIR}")
    message(STATUS "  Lib: ${ONNXRUNTIME_LIB}")
    target_include_directories(tts PUBLIC ${ONNXRUNTIME_INCLUDE_DIR})
    target_link_libraries(tts PUBLIC ${ONNXRUNTIME_LIB})
else()
    # 先尝试直接查找（避免损坏的 CMake 配置文件问题）
    if(APPLE)
        # macOS Homebrew
        find_library(ONNXRUNTIME_LIB_AUTO onnxruntime HINTS /opt/homebrew/lib /usr/local/lib)
        find_path(ONNXRUNTIME_INCLUDE_AUTO onnxruntime_cxx_api.h HINTS /opt/homebrew/include /usr/local/include)
    else()
        # Linux 默认路径
        find_library(ONNXRUNTIME_LIB_AUTO onnxruntime HINTS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
        find_path(ONNXRUNTIME_INCLUDE_AUTO onnxruntime_cxx_api.h HINTS /usr/include /usr/local/include /usr/include/onnxruntime)
    endif()

    if(ONNXRUNTIME_LIB_AUTO AND ONNXRUNTIME_INCLUDE_AUTO)
        message(STATUS "[tts] Found ONNX Runtime via direct search:")
        message(STATUS "  Include: ${ONNXRUNTIME_INCLUDE_AUTO}")
        message(STATUS "  Lib: ${ONNXRUNTIME_LIB_AUTO}")
        target_link_libraries(tts PUBLIC ${ONNXRUNTIME_LIB_AUTO})
        target_include_directories(tts PUBLIC ${ONNXRUNTIME_INCLUDE_AUTO})
    else()
        # 直接查找失败，尝试 find_package 作为备选
        find_package(onnxruntime QUIET)
        if(onnxruntime_FOUND)
            message(STATUS "[tts] Found ONNX Runtime via find_package")
            target_link_libraries(tts PUBLIC onnxruntime::onnxruntime)
        else()
            message(WARNING "[tts] ONNX Runtime not found! Please specify:")
            message(WARNING "  -DONNXRUNTIME_INCLUDE_DIR=/path/to/include")
            message(WARNING "  -DONNXRUNTIME_LIB=/path/to/libonnxruntime.so")
        endif()
    endif()
endif()

# FFTW3 (for audio processing)
if(APPLE)
    # On macOS, use find_library for full paths (avoids stale Cellar paths from pkg-config)
    find_library(TTS_FFTW3_LIB fftw3 HINTS /opt/homebrew/lib /opt/homebrew/opt/fftw/lib /usr/local/lib)
    find_library(TTS_FFTW3F_LIB fftw3f HINTS /opt/homebrew/lib /opt/homebrew/opt/fftw/lib /usr/local/lib)
    find_path(TTS_FFTW3_INCLUDE fftw3.h HINTS /opt/homebrew/include /opt/homebrew/opt/fftw/include /usr/local/include)

    if(TTS_FFTW3_LIB)
        target_link_libraries(tts PUBLIC ${TTS_FFTW3_LIB})
    endif()
    if(TTS_FFTW3F_LIB)
        target_link_libraries(tts PUBLIC ${TTS_FFTW3F_LIB})
    endif()
    if(TTS_FFTW3_INCLUDE)
        target_include_directories(tts PUBLIC ${TTS_FFTW3_INCLUDE})
    endif()
    if(NOT TTS_FFTW3_LIB AND NOT TTS_FFTW3F_LIB)
        message(WARNING "FFTW3 not found on macOS")
    endif()
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FFTW3 QUIET fftw3)
        pkg_check_modules(FFTW3F QUIET fftw3f)
    endif()

    if(FFTW3_FOUND)
        target_link_directories(tts PUBLIC ${FFTW3_LIBRARY_DIRS})
        target_link_libraries(tts PUBLIC ${FFTW3_LIBRARIES})
        target_include_directories(tts PUBLIC ${FFTW3_INCLUDE_DIRS})
    endif()

    if(FFTW3F_FOUND)
        target_link_directories(tts PUBLIC ${FFTW3F_LIBRARY_DIRS})
        target_link_libraries(tts PUBLIC ${FFTW3F_LIBRARIES})
    endif()

    if(NOT FFTW3_FOUND AND NOT FFTW3F_FOUND)
        message(WARNING "FFTW3 not found")
    endif()
endif()

# libcurl (for model download)
find_package(CURL QUIET)
if(CURL_FOUND)
    target_link_libraries(tts PRIVATE CURL::libcurl)
endif()

# =============================================================================
# 第三方库 Include 目录
# =============================================================================

# cppjieba include
target_include_directories(tts PRIVATE
    ${CPPJIEBA_DIR}/include
    ${CPPJIEBA_DIR}/deps/limonp/include
)

# cpp-pinyin include
target_include_directories(tts PRIVATE
    ${CPP_PINYIN_DIR}/include
    ${CPP_PINYIN_DIR}/src
    ${CPP_PINYIN_DIR}/src/toneUtil
)

# espeak-ng (英文音素)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(ESPEAK QUIET espeak-ng)
    if(ESPEAK_FOUND)
        target_link_directories(tts PUBLIC ${ESPEAK_LIBRARY_DIRS})
        target_link_libraries(tts PUBLIC ${ESPEAK_LIBRARIES})
        target_include_directories(tts PRIVATE ${ESPEAK_INCLUDE_DIRS})
    else()
        # macOS Homebrew fallback
        if(APPLE)
            find_library(ESPEAK_LIB espeak-ng HINTS
                /opt/homebrew/opt/espeak-ng/lib
                /opt/homebrew/Cellar/espeak-ng/1.52.0/lib
                /opt/homebrew/lib
                /usr/local/lib)
            find_path(ESPEAK_INCLUDE espeak-ng/speak_lib.h HINTS
                /opt/homebrew/opt/espeak-ng/include
                /opt/homebrew/Cellar/espeak-ng/1.52.0/include
                /opt/homebrew/include
                /usr/local/include)
            if(ESPEAK_LIB AND ESPEAK_INCLUDE)
                target_link_libraries(tts PUBLIC ${ESPEAK_LIB})
                target_include_directories(tts PRIVATE ${ESPEAK_INCLUDE})
            else()
                message(WARNING "espeak-ng not found")
            endif()
        endif()
    endif()
endif()

# =============================================================================
# 导出
# =============================================================================

# 安装头文件
install(DIRECTORY inc/ DESTINATION include/tts)

# 安装库
install(TARGETS tts
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# =============================================================================
# 测试程序
# =============================================================================

option(BUILD_SIMPLE_DEMO "Build TTS tests" ON)
if(BUILD_SIMPLE_DEMO)
    add_executable(simple_demo examples/simple_demo.cpp)
    target_link_libraries(simple_demo PRIVATE tts)

    # 流式 TTS 演示程序 (需要 evo_audio / portaudio)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PORTAUDIO QUIET portaudio-2.0)
    endif()

    if(PORTAUDIO_FOUND)
        # evo_audio 源文件路径
        set(EVO_AUDIO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../audio)

        if(EXISTS ${EVO_AUDIO_DIR}/src/evo_audio.cpp)
            add_executable(streaming_tts_demo
                examples/streaming_tts_demo.cpp
                ${EVO_AUDIO_DIR}/src/evo_audio.cpp
                ${EVO_AUDIO_DIR}/src/audio_stream.cpp
                ${EVO_AUDIO_DIR}/src/resampler.cpp
                ${EVO_AUDIO_DIR}/src/resampler_rvv.cpp
            )

            target_include_directories(streaming_tts_demo PRIVATE
                ${EVO_AUDIO_DIR}/inc
                ${PORTAUDIO_INCLUDE_DIRS}
            )

            target_include_directories(streaming_tts_demo PRIVATE
                ${EVO_AUDIO_DIR}/inc/internal
            )

            target_link_libraries(streaming_tts_demo PRIVATE
                tts
                ${PORTAUDIO_LIBRARIES}
                Threads::Threads
            )
            target_link_directories(streaming_tts_demo PRIVATE ${PORTAUDIO_LIBRARY_DIRS})

            find_package(Threads REQUIRED)

            message(STATUS "[tts] streaming_tts_demo enabled")
        else()
            message(STATUS "[tts] streaming_tts_demo disabled (evo_audio source not found)")
        endif()
    else()
        message(STATUS "[tts] streaming_tts_demo disabled (PortAudio not found)")
    endif()
endif()

# =============================================================================
# Python Bindings (optional)
# =============================================================================

option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)

if(BUILD_PYTHON_BINDINGS)
    # 1. Find Python interpreter and development libraries
    find_package(Python3 COMPONENTS Interpreter Development QUIET)

    if(Python3_FOUND)
        message(STATUS "[tts] Python found: ${Python3_EXECUTABLE} (${Python3_VERSION})")
    else()
        message(STATUS "[tts] Python bindings disabled (Python3 not found)")
    endif()

    # 2. Find pybind11 (try system first, then Python module)
    find_package(pybind11 CONFIG QUIET)

    if(NOT pybind11_FOUND AND Python3_FOUND)
        # Try to get pybind11 from Python module
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import pybind11; print(pybind11.get_cmake_dir())"
            OUTPUT_VARIABLE PYBIND11_CMAKE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE PYBIND11_RESULT
        )
        if(PYBIND11_RESULT EQUAL 0)
            list(APPEND CMAKE_PREFIX_PATH "${PYBIND11_CMAKE_DIR}")
            find_package(pybind11 CONFIG QUIET)
        endif()
    endif()

    # 3. Build Python bindings module
    if(pybind11_FOUND)
        pybind11_add_module(_evo_tts ${CMAKE_CURRENT_SOURCE_DIR}/python/tts_bindings.cpp)

        target_include_directories(_evo_tts PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
        )

        target_link_libraries(_evo_tts PRIVATE tts)

        set_target_properties(_evo_tts PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/python
        )

        message(STATUS "[tts] Python bindings enabled (_evo_tts module for Python ${Python3_VERSION})")

        # 4. Get Python site-packages path for installation
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('purelib'))"
            OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # 5. Custom install target: make tts-install-python
        add_custom_target(tts-install-python
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${PYTHON_SITE_PACKAGES}/evo_tts
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/python/evo_tts
                ${PYTHON_SITE_PACKAGES}/evo_tts
            COMMAND ${CMAKE_COMMAND} -E copy
                $<TARGET_FILE:_evo_tts>
                ${PYTHON_SITE_PACKAGES}/evo_tts/
            DEPENDS _evo_tts
            COMMENT "Installing evo_tts to ${PYTHON_SITE_PACKAGES}/evo_tts/"
        )
    else()
        message(STATUS "[tts] Python bindings disabled (pybind11 not found)")
        message(STATUS "  Install pybind11: pip install pybind11")
    endif()
endif()
