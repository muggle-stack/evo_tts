#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "tts_api.hpp"

namespace py = pybind11;

// =============================================================================
// PyTtsCallback - Python 回调包装类
// =============================================================================

/**
 * @brief Python 回调适配器
 *
 * 继承 TtsResultCallback，将 C++ 回调桥接到 Python 函数
 * 关键点：在调用 Python 回调时获取 GIL
 */
class PyTtsCallback : public Evo::TtsResultCallback {
public:
    using OpenCallback = std::function<void()>;
    using EventCallback = std::function<void(std::shared_ptr<Evo::TtsEngineResult>)>;
    using CompleteCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using CloseCallback = std::function<void()>;

    PyTtsCallback() = default;
    ~PyTtsCallback() override = default;

    // 设置回调
    void setOnOpen(OpenCallback cb) { on_open_ = std::move(cb); }
    void setOnEvent(EventCallback cb) { on_event_ = std::move(cb); }
    void setOnComplete(CompleteCallback cb) { on_complete_ = std::move(cb); }
    void setOnError(ErrorCallback cb) { on_error_ = std::move(cb); }
    void setOnClose(CloseCallback cb) { on_close_ = std::move(cb); }

    // 重写基类虚函数
    void OnOpen() override {
        if (on_open_) {
            py::gil_scoped_acquire acquire;  // 获取 GIL
            on_open_();
        }
    }

    void OnEvent(std::shared_ptr<Evo::TtsEngineResult> result) override {
        if (on_event_) {
            py::gil_scoped_acquire acquire;  // 获取 GIL
            on_event_(result);
        }
    }

    void OnComplete() override {
        if (on_complete_) {
            py::gil_scoped_acquire acquire;  // 获取 GIL
            on_complete_();
        }
    }

    void OnError(const std::string& message) override {
        if (on_error_) {
            py::gil_scoped_acquire acquire;  // 获取 GIL
            on_error_(message);
        }
    }

    void OnClose() override {
        if (on_close_) {
            py::gil_scoped_acquire acquire;  // 获取 GIL
            on_close_();
        }
    }

private:
    OpenCallback on_open_;
    EventCallback on_event_;
    CompleteCallback on_complete_;
    ErrorCallback on_error_;
    CloseCallback on_close_;
};

// =============================================================================
// pybind11 模块定义
// =============================================================================

PYBIND11_MODULE(_evo_tts, m) {
    m.doc() = "EvoTTS - Text-To-Speech Engine Python bindings";

    // =========================================================================
    // 枚举类型
    // =========================================================================

    py::enum_<Evo::AudioFormat>(m, "AudioFormat", "Audio format types")
        .value("PCM", Evo::AudioFormat::PCM, "Raw PCM data")
        .value("WAV", Evo::AudioFormat::WAV, "WAV file format")
        .value("MP3", Evo::AudioFormat::MP3, "MP3 format (reserved)")
        .value("OGG", Evo::AudioFormat::OGG, "OGG format (reserved)")
        .export_values();

    py::enum_<Evo::BackendType>(m, "BackendType", "TTS backend types")
        .value("MATCHA_ZH", Evo::BackendType::MATCHA_ZH,
            "Matcha Chinese (22050Hz)")
        .value("MATCHA_EN", Evo::BackendType::MATCHA_EN,
            "Matcha English (22050Hz)")
        .value("MATCHA_ZH_EN", Evo::BackendType::MATCHA_ZH_EN,
            "Matcha Chinese-English bilingual (16000Hz)")
        .value("COSYVOICE", Evo::BackendType::COSYVOICE, "CosyVoice (reserved)")
        .value("VITS", Evo::BackendType::VITS, "VITS (reserved)")
        .value("PIPER", Evo::BackendType::PIPER, "Piper TTS (reserved)")
        .value("KOKORO", Evo::BackendType::KOKORO, "Kokoro TTS (reserved)")
        .export_values();

    // =========================================================================
    // TtsConfig - 配置结构
    // =========================================================================

    py::class_<Evo::TtsConfig>(m, "TtsConfig", "TTS engine configuration")
        .def(py::init<>(), "Create default configuration")

        // 字段（readwrite）
        .def_readwrite("backend", &Evo::TtsConfig::backend, "Backend type")
        .def_readwrite("model", &Evo::TtsConfig::model, "Model name")
        .def_readwrite("model_dir", &Evo::TtsConfig::model_dir, "Model directory path")
        .def_readwrite("voice", &Evo::TtsConfig::voice, "Voice name")
        .def_readwrite("speaker_id", &Evo::TtsConfig::speaker_id, "Speaker ID (multi-speaker models)")
        .def_readwrite("format", &Evo::TtsConfig::format, "Output audio format")
        .def_readwrite("sample_rate", &Evo::TtsConfig::sample_rate, "Output sample rate (Hz)")
        .def_readwrite("volume", &Evo::TtsConfig::volume, "Volume [0, 100]")
        .def_readwrite("speech_rate", &Evo::TtsConfig::speech_rate, "Speech rate (>1.0 fast, <1.0 slow)")
        .def_readwrite("pitch", &Evo::TtsConfig::pitch, "Pitch")
        .def_readwrite("target_rms", &Evo::TtsConfig::target_rms, "Target RMS level")
        .def_readwrite("compression_ratio", &Evo::TtsConfig::compression_ratio, "Compression ratio")
        .def_readwrite("use_rms_norm", &Evo::TtsConfig::use_rms_norm, "Use RMS normalization")
        .def_readwrite("remove_clicks", &Evo::TtsConfig::remove_clicks, "Remove clicks")
        .def_readwrite("num_threads", &Evo::TtsConfig::num_threads, "Number of inference threads")
        .def_readwrite("enable_warmup", &Evo::TtsConfig::enable_warmup, "Enable warmup on startup")

        // 静态工厂方法
        .def_static("Default", &Evo::TtsConfig::Default,
                    "Create default configuration (Chinese)")
        .def_static("MatchaZH", &Evo::TtsConfig::MatchaZH,
                    py::arg("model_dir") = "~/.cache/matcha-tts",
                    "Create Matcha Chinese configuration")
        .def_static("MatchaEN", &Evo::TtsConfig::MatchaEN,
                    py::arg("model_dir") = "~/.cache/matcha-tts",
                    "Create Matcha English configuration")
        .def_static("MatchaZHEN", &Evo::TtsConfig::MatchaZHEN,
                    py::arg("model_dir") = "~/.cache/matcha-tts",
                    "Create Matcha Chinese-English bilingual configuration")

        // Builder 方法（链式调用）
        .def("withSpeed", &Evo::TtsConfig::withSpeed,
            py::arg("speed"),
            "Set speech rate (chainable)")
        .def("withSpeaker", &Evo::TtsConfig::withSpeaker,
            py::arg("id"),
            "Set speaker ID (chainable)")
        .def("withVolume", &Evo::TtsConfig::withVolume,
            py::arg("vol"),
            "Set volume (chainable)")

        // 字符串表示
        .def("__repr__", [](const Evo::TtsConfig& config) {
            return "<TtsConfig backend=" + std::to_string(static_cast<int>(config.backend)) +
                " model='" + config.model + "'" +
                " sample_rate=" + std::to_string(config.sample_rate) + ">";
        });

    // =========================================================================
    // TtsEngineResult - 合成结果
    // =========================================================================

    py::class_<Evo::TtsEngineResult, std::shared_ptr<Evo::TtsEngineResult>>(
        m, "TtsEngineResult", "TTS synthesis result")

        // 音频数据获取
        .def("get_audio_data", &Evo::TtsEngineResult::GetAudioData,
            "Get audio as raw bytes (uint8[])")
        .def("get_audio_float", &Evo::TtsEngineResult::GetAudioFloat,
            "Get audio as float32 array [-1.0, 1.0]")
        .def("get_audio_int16", &Evo::TtsEngineResult::GetAudioInt16,
            "Get audio as int16 array (PCM)")

        // 元信息
        .def("get_timestamp", &Evo::TtsEngineResult::GetTimestamp,
            "Get timestamp information (JSON string)")
        .def("get_response", &Evo::TtsEngineResult::GetResponse,
            "Get full response (JSON string)")
        .def("get_request_id", &Evo::TtsEngineResult::GetRequestId,
            "Get request ID")

        // 状态检查
        .def("is_success", &Evo::TtsEngineResult::IsSuccess,
            "Check if synthesis succeeded")
        .def("get_code", &Evo::TtsEngineResult::GetCode,
            "Get error code")
        .def("get_message", &Evo::TtsEngineResult::GetMessage,
            "Get error message")
        .def("is_empty", &Evo::TtsEngineResult::IsEmpty,
            "Check if result is empty (no audio)")
        .def("is_sentence_end", &Evo::TtsEngineResult::IsSentenceEnd,
            "Check if this is the end of a sentence (streaming mode)")

        // 性能指标
        .def("get_sample_rate", &Evo::TtsEngineResult::GetSampleRate,
            "Get sample rate (Hz)")
        .def("get_duration_ms", &Evo::TtsEngineResult::GetDurationMs,
            "Get audio duration (milliseconds)")
        .def("get_processing_time_ms", &Evo::TtsEngineResult::GetProcessingTimeMs,
            "Get processing time (milliseconds)")
        .def("get_rtf", &Evo::TtsEngineResult::GetRTF,
            "Get Real-Time Factor (processing_time / audio_duration)")

        // 文件操作
        .def("save_to_file", &Evo::TtsEngineResult::SaveToFile,
            py::arg("file_path"),
            "Save audio to file")

        // Python 魔术方法
        .def("__bool__", [](const Evo::TtsEngineResult& r) {
            return !r.IsEmpty();
        }, "Check if result has audio content")
        .def("__repr__", [](const Evo::TtsEngineResult& r) {
            return "<TtsEngineResult " +
                std::string(r.IsSuccess() ? "success" : "failed") +
                " duration=" + std::to_string(r.GetDurationMs()) + "ms" +
                " rtf=" + std::to_string(r.GetRTF()) + ">";
        });

    // =========================================================================
    // TtsResultCallback - 基类（先绑定基类）
    // =========================================================================

    py::class_<Evo::TtsResultCallback, std::shared_ptr<Evo::TtsResultCallback>>(
        m, "TtsResultCallbackBase", "Base callback interface for TTS")
        .def("OnOpen", &Evo::TtsResultCallback::OnOpen)
        .def("OnEvent", &Evo::TtsResultCallback::OnEvent)
        .def("OnComplete", &Evo::TtsResultCallback::OnComplete)
        .def("OnError", &Evo::TtsResultCallback::OnError)
        .def("OnClose", &Evo::TtsResultCallback::OnClose);

    // =========================================================================
    // PyTtsCallback - 回调包装（派生类，声明继承关系）
    // =========================================================================

    py::class_<PyTtsCallback, Evo::TtsResultCallback, std::shared_ptr<PyTtsCallback>>(
        m, "TtsCallback", "Callback for streaming synthesis")
        .def(py::init<>(), "Create callback object")

        // 设置回调函数
        .def("on_open", &PyTtsCallback::setOnOpen,
            py::arg("callback"),
            "Set callback for connection open")
        .def("on_event", &PyTtsCallback::setOnEvent,
            py::arg("callback"),
            "Set callback for audio events (main callback)")
        .def("on_complete", &PyTtsCallback::setOnComplete,
            py::arg("callback"),
            "Set callback for completion")
        .def("on_error", &PyTtsCallback::setOnError,
            py::arg("callback"),
            "Set callback for errors")
        .def("on_close", &PyTtsCallback::setOnClose,
            py::arg("callback"),
            "Set callback for connection close");

    // =========================================================================
    // TtsEngine - 主引擎
    // =========================================================================

    py::class_<Evo::TtsEngine>(m, "TtsEngine", "TTS Engine - Main synthesis interface")
        // 构造函数
        .def(py::init<Evo::BackendType, const std::string&>(),
            py::arg("backend") = Evo::BackendType::MATCHA_ZH,
            py::arg("model_dir") = "",
            "Create TTS engine with backend type")
        .def(py::init<const Evo::TtsConfig&>(),
            py::arg("config"),
            "Create TTS engine with configuration")

        // 非流式调用（阻塞）- 释放 GIL
        .def("call", [](Evo::TtsEngine& self, const std::string& text) {
            py::gil_scoped_release release;  // 释放 GIL，允许其他 Python 线程运行
            return self.Call(text);
        }, py::arg("text"),
            "Synthesize text (blocking, releases GIL)")

        .def("call_with_config", [](Evo::TtsEngine& self,
            const std::string& text,
            const Evo::TtsConfig& config) {
            py::gil_scoped_release release;
            return self.Call(text, config);
        }, py::arg("text"), py::arg("config"),
            "Synthesize text with custom config (blocking, releases GIL)")

        .def("call_to_file", [](Evo::TtsEngine& self,
            const std::string& text,
            const std::string& file_path) {
            py::gil_scoped_release release;
            return self.CallToFile(text, file_path);
        }, py::arg("text"), py::arg("file_path"),
            "Synthesize text and save to file (blocking, releases GIL)")

        // 流式调用
        .def("streaming_call", &Evo::TtsEngine::StreamingCall,
            py::arg("text"),
            py::arg("callback"),
            py::arg("config") = Evo::TtsConfig(),
            "Streaming synthesis with callback")

        // 注意：DuplexStream 暂不绑定（需要额外的包装类）

        // 动态配置
        .def("set_speed", &Evo::TtsEngine::SetSpeed,
            py::arg("speed"),
            "Set speech rate (>1.0 fast, <1.0 slow)")
        .def("set_speaker", &Evo::TtsEngine::SetSpeaker,
            py::arg("speaker_id"),
            "Set speaker ID")
        .def("set_volume", &Evo::TtsEngine::SetVolume,
            py::arg("volume"),
            "Set volume [0, 100]")
        .def("get_config", &Evo::TtsEngine::GetConfig,
            "Get current configuration")

        // 辅助方法
        .def("is_initialized", &Evo::TtsEngine::IsInitialized,
            "Check if engine is initialized")
        .def("get_engine_name", &Evo::TtsEngine::GetEngineName,
            "Get engine type name")
        .def("get_backend_type", &Evo::TtsEngine::GetBackendType,
            "Get backend type")
        .def("get_num_speakers", &Evo::TtsEngine::GetNumSpeakers,
            "Get number of supported speakers")
        .def("get_sample_rate", &Evo::TtsEngine::GetSampleRate,
            "Get output sample rate (Hz)")
        .def("get_last_request_id", &Evo::TtsEngine::GetLastRequestId,
            "Get last request ID")

        // 字符串表示
        .def("__repr__", [](const Evo::TtsEngine& engine) {
            return "<TtsEngine backend=" + engine.GetEngineName() +
                " sample_rate=" + std::to_string(engine.GetSampleRate()) + "Hz" +
                " initialized=" + (engine.IsInitialized() ? "true" : "false") + ">";
        });

    // =========================================================================
    // 模块级属性
    // =========================================================================

    m.attr("__version__") = "1.0.0";
    m.attr("__author__") = "muggle";
    m.attr("__doc__") = "EvoTTS - Text-To-Speech Engine Python bindings";
}
