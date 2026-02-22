# TTS - 离线语音合成引擎

基于 Matcha-TTS 和 Kokoro 的多后端离线语音合成框架，支持中/英/混合语音，提供 C++ 和 Python 接口。

## 特性

- 多后端架构：Matcha-TTS（中/英/中英混合）、Kokoro（多音色）
- 流式合成（回调模式）与非流式合成（阻塞模式）
- 文本正规化：数字、日期、货币自动转换为语音文本
- 中文分词（cppjieba）+ 拼音转换（cpp-pinyin）+ IPA 音素
- 音频后处理：RMS 归一化、动态范围压缩、爆音消除
- Python 绑定（pybind11），支持 NumPy 音频数组

## 依赖

### 系统依赖

| 平台 | 安装命令 |
|------|---------|
| Ubuntu/Debian | `sudo apt install libfftw3-dev espeak-ng libcurl4-openssl-dev` |
| macOS | `brew install fftw espeak curl onnxruntime` |

ONNX Runtime 需手动安装或通过包管理器安装。可选：`pybind11`（Python 绑定）。

cppjieba 和 cpp-pinyin 由 CMake 自动克隆到 `~/.cache/`。

### 模型文件

首次运行自动下载到 `~/.cache/matcha-tts/`：

| 文件 | 说明 |
|------|------|
| `matcha-icefall-zh-baker/` | 中文声学模型 |
| `matcha-icefall-en_US-ljspeech/` | 英文声学模型 |
| `matcha-icefall-zh-en/` | 中英混合声学模型 |
| `vocos-22khz-univ.onnx` | 声码器（中文/英文） |
| `vocos-16khz-univ.onnx` | 声码器（中英混合） |

## 编译

```bash
mkdir -p build && cd build
cmake .. && make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
```

编译产物：

```
build/lib/libtts.a                # TTS 静态库
build/bin/simple_demo             # 简单合成示例
build/bin/streaming_tts_demo      # 流式合成示例（需 audio 模块）
```

## 快速开始

### C++

```cpp
#include "tts_api.hpp"

int main() {
    auto config = Evo::TtsConfig::MatchaZH();
    auto engine = std::make_shared<Evo::TtsEngine>(config);
    auto result = engine->Call("你好世界");
    if (result && result->IsSuccess()) {
        result->SaveToFile("output.wav");
    }
    return 0;
}
```

### Python

```bash
cd modules/tts/python && pip install -e .
```

```python
import evo_tts

result = evo_tts.synthesize("你好世界")
result.save("output.wav")
print(f"时长: {result.duration_ms}ms, RTF: {result.rtf:.3f}")
```

## 示例

```bash
# C++ 简单合成（交互模式）
./build/bin/simple_demo
./build/bin/simple_demo -p "你好世界" -o output.wav

# C++ 切换后端
./build/bin/simple_demo -l matcha:en        # 英文
./build/bin/simple_demo -l matcha:zh-en     # 中英混合
./build/bin/simple_demo -l kokoro           # Kokoro 默认音色
./build/bin/simple_demo -l kokoro:yunxi     # Kokoro 指定音色

# C++ 流式合成（需 audio 模块）
./build/bin/streaming_tts_demo -p "你好。今天天气很好。"
./build/bin/streaming_tts_demo -l zh-en --no-play

# Python
python python/examples/simple_demo.py
python python/examples/streaming_demo.py -p "测试文本"
```

## API 概览

### 核心类（命名空间 `Evo::`）

| 类 | 说明 |
|----|------|
| `TtsEngine` | 语音合成引擎，支持阻塞/流式/双向流合成 |
| `TtsConfig` | 引擎配置，提供 `MatchaZH()`/`MatchaEN()`/`MatchaZHEN()`/`Kokoro()` 工厂方法 |
| `TtsEngineResult` | 合成结果，含音频数据（float/int16/bytes）、时长、RTF |
| `TtsResultCallback` | 流式合成回调接口（OnOpen/OnEvent/OnComplete/OnError/OnClose） |

### 关键方法

| 方法 | 说明 |
|------|------|
| `Call(text)` | 非流式合成（阻塞） |
| `CallToFile(text, path)` | 直接合成到文件 |
| `StreamingCall(text, callback)` | 流式合成，按句子回调 |
| `StartDuplexStream(callback)` | 双向流：边输入文本边合成 |

详细文档见 [API.md](API.md)

## 支持后端

| 后端 | 配置工厂方法 | 语言 | 采样率 | 说明 |
|------|-------------|------|--------|------|
| `MATCHA_ZH` | `TtsConfig::MatchaZH()` | 中文 | 22050Hz | 默认后端 |
| `MATCHA_EN` | `TtsConfig::MatchaEN()` | 英文 | 22050Hz | LJSpeech |
| `MATCHA_ZH_EN` | `TtsConfig::MatchaZHEN()` | 中英混合 | 16000Hz | 混合模型 |
| `KOKORO` | `TtsConfig::Kokoro()` | 中/英 | 24000Hz | 多音色（中文 8 声、英文 30+ 声） |

## 配置参数

### TtsConfig

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `backend` | `BackendType` | `MATCHA_ZH` | 后端类型 |
| `model_dir` | `string` | `"~/.cache/matcha-tts"` | 模型目录 |
| `voice` | `string` | `"default"` | 音色名称（Kokoro） |
| `speaker_id` | `int` | `0` | 说话人 ID |
| `sample_rate` | `int` | `22050` | 输出采样率 (Hz) |
| `speech_rate` | `float` | `1.0` | 语速（>1.0 加速，<1.0 减速） |
| `volume` | `int` | `50` | 音量 [0, 100] |
| `target_rms` | `float` | `0.15` | RMS 归一化电平 |
| `num_threads` | `int` | `2` | ONNX 推理线程数 |

## CMake 集成

```cmake
add_subdirectory(modules/tts)
target_link_libraries(your_target PRIVATE tts)
target_include_directories(your_target PRIVATE ${TTS_SOURCE_DIR}/inc)
```

## 许可证

MIT License - 详见 [LICENSE](LICENSE)
