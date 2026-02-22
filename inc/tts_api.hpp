#ifndef TTS_API_HPP
#define TTS_API_HPP

/**
 * EvoTtsSDK - Evo TTS Engine SDK
 *
 * 语音合成引擎适配层，提供统一的 C++ 接口。
 *
 * 使用示例 1 - 文本合成（阻塞模式）:
 *
 *   auto engine = std::make_shared<Evo::TtsEngine>();
 *   auto result = engine->Call("你好世界");
 *   if (result && result->IsSuccess()) {
 *       auto audio = result->GetAudioData();
 *       // 播放或保存音频...
 *   }
 *
 * 使用示例 2 - 带配置的初始化:
 *
 *   Evo::TtsConfig config = Evo::TtsConfig::MatchaZH();
 *   config.speech_rate = 1.2f;
 *   auto engine = std::make_shared<Evo::TtsEngine>(config);
 *
 * 使用示例 3 - 流式合成:
 *
 *   auto engine = std::make_shared<Evo::TtsEngine>();
 *   engine->StreamingCall("你好世界，今天天气很好。", callback);
 *
 * 使用示例 4 - 双向流（边输入边合成）:
 *
 *   auto stream = engine->StartDuplexStream(callback);
 *   stream->SendText("第一句话。");
 *   stream->SendText("第二句话。");
 *   stream->Complete();
 */

#include <cstdint>

#include <functional>
#include <memory>
#include <string>
#include <vector>

// Forward declaration of internal types
namespace tts {
    class ITtsBackend;
    struct AudioChunk;
    struct ErrorInfo;
}  // namespace tts

namespace Evo {

// =============================================================================
// AudioFormat - 音频格式
// =============================================================================

enum class AudioFormat {
    PCM,  ///< 原始 PCM 数据
    WAV,  ///< WAV 文件格式
    MP3,  ///< MP3 格式（预留）
    OGG,  ///< OGG 格式（预留）
};

// =============================================================================
// BackendType - 后端类型
// =============================================================================

enum class BackendType {
    // Matcha-TTS 系列
    MATCHA_ZH,          ///< 中文 (matcha-icefall-zh-baker, 22050Hz)
    MATCHA_EN,          ///< 英文 (matcha-icefall-en_US-ljspeech, 22050Hz)
    MATCHA_ZH_EN,       ///< 中英混合 (matcha-icefall-zh-en, 16000Hz)

    // 预留扩展
    COSYVOICE,          ///< CosyVoice
    VITS,               ///< VITS
    PIPER,              ///< Piper TTS
    KOKORO,             ///< Kokoro TTS
    CUSTOM,             ///< 自定义后端
};

// =============================================================================
// TtsConfig - TTS 配置
// =============================================================================

/**
 * @brief TTS 引擎配置
 */
struct TtsConfig {
    // -------------------------------------------------------------------------
    // 后端选择
    // -------------------------------------------------------------------------

    BackendType backend = BackendType::MATCHA_ZH;  ///< 后端类型

    // -------------------------------------------------------------------------
    // 模型配置
    // -------------------------------------------------------------------------

    std::string model;                  ///< 模型名称
    std::string model_dir;              ///< 模型目录路径，空则使用默认路径
    std::string voice = "default";      ///< 音色名称
    int speaker_id = 0;                 ///< 说话人ID (多说话人模型)

    // -------------------------------------------------------------------------
    // 音频参数
    // -------------------------------------------------------------------------

    AudioFormat format = AudioFormat::WAV;  ///< 输出格式
    int sample_rate = 22050;            ///< 输出采样率 (Hz)
    int volume = 50;                    ///< 音量 [0, 100]

    // -------------------------------------------------------------------------
    // 合成参数
    // -------------------------------------------------------------------------

    float speech_rate = 1.0f;           ///< 语速 (>1.0快, <1.0慢)
    float pitch = 1.0f;                 ///< 音调

    // -------------------------------------------------------------------------
    // 音频后处理
    // -------------------------------------------------------------------------

    float target_rms = 0.15f;           ///< 目标RMS电平
    float compression_ratio = 2.0f;     ///< 压缩比
    bool use_rms_norm = true;           ///< 使用RMS归一化
    bool remove_clicks = true;          ///< 移除爆音

    // -------------------------------------------------------------------------
    // 性能配置
    // -------------------------------------------------------------------------

    int num_threads = 2;                ///< 推理线程数
    bool enable_warmup = true;          ///< 启动时预热

    // -------------------------------------------------------------------------
    // 便捷构建方法
    // -------------------------------------------------------------------------

    /// @brief 创建默认配置（中文）
    static TtsConfig Default() {
        return TtsConfig();
    }

    /// @brief 创建 Matcha 中文配置
    /// @param model_dir 模型目录路径
    static TtsConfig MatchaZH(const std::string& model_dir = "~/.cache/matcha-tts") {
        TtsConfig config;
        config.backend = BackendType::MATCHA_ZH;
        config.model = "matcha-icefall-zh-baker";
        config.model_dir = model_dir;
        config.sample_rate = 22050;
        return config;
    }

    /// @brief 创建 Matcha 英文配置
    /// @param model_dir 模型目录路径
    static TtsConfig MatchaEN(const std::string& model_dir = "~/.cache/matcha-tts") {
        TtsConfig config;
        config.backend = BackendType::MATCHA_EN;
        config.model = "matcha-icefall-en_US-ljspeech";
        config.model_dir = model_dir;
        config.sample_rate = 22050;
        return config;
    }

    /// @brief 创建 Matcha 中英混合配置
    /// @param model_dir 模型目录路径
    static TtsConfig MatchaZHEN(const std::string& model_dir = "~/.cache/matcha-tts") {
        TtsConfig config;
        config.backend = BackendType::MATCHA_ZH_EN;
        config.model = "matcha-icefall-zh-en";
        config.model_dir = model_dir;
        config.sample_rate = 16000;  // zh-en 使用 16kHz
        return config;
    }

    /// @brief 创建 Kokoro 中文配置
    /// @param model_dir 模型目录路径
    /// @param voice 音色名称
    static TtsConfig Kokoro(const std::string& model_dir = "~/.cache/kokoro-tts",
                            const std::string& voice = "default") {
        TtsConfig config;
        config.backend = BackendType::KOKORO;
        config.model = "kokoro-v1.0";
        config.model_dir = model_dir;
        config.voice = voice;
        config.sample_rate = 24000;
        return config;
    }

    // 链式配置
    TtsConfig withSpeed(float speed) const {
        auto c = *this;
        c.speech_rate = speed;
        return c;
    }

    TtsConfig withSpeaker(int id) const {
        auto c = *this;
        c.speaker_id = id;
        return c;
    }

    TtsConfig withVolume(int vol) const {
        auto c = *this;
        c.volume = vol;
        return c;
    }
};

// =============================================================================
// TtsEngineResult - 合成结果
// =============================================================================

class TtsEngineResult {
public:
    TtsEngineResult();
    ~TtsEngineResult();

    // 禁止拷贝，允许移动
    TtsEngineResult(const TtsEngineResult&) = delete;
    TtsEngineResult& operator=(const TtsEngineResult&) = delete;
    TtsEngineResult(TtsEngineResult&&) noexcept;
    TtsEngineResult& operator=(TtsEngineResult&&) noexcept;

    // -------------------------------------------------------------------------
    // 音频数据获取
    // -------------------------------------------------------------------------

    /// @brief 获取音频数据（字节格式，用于播放或保存）
    /// @return 音频数据
    std::vector<uint8_t> GetAudioData() const;

    /// @brief 获取音频数据（float格式，[-1.0, 1.0]）
    /// @return 音频数据
    std::vector<float> GetAudioFloat() const;

    /// @brief 获取音频数据（int16格式）
    /// @return 音频数据
    std::vector<int16_t> GetAudioInt16() const;

    // -------------------------------------------------------------------------
    // 元信息
    // -------------------------------------------------------------------------

    /// @brief 获取时间戳信息（JSON格式）
    /// @return 时间戳字符串
    std::string GetTimestamp() const;

    /// @brief 获取完整响应（JSON格式）
    /// @return JSON 响应字符串
    std::string GetResponse() const;

    /// @brief 获取请求 ID
    /// @return 请求标识符
    std::string GetRequestId() const;

    // -------------------------------------------------------------------------
    // 状态检查
    // -------------------------------------------------------------------------

    /// @brief 是否合成成功
    /// @return true 表示成功
    bool IsSuccess() const;

    /// @brief 获取错误码
    /// @return 错误码字符串
    std::string GetCode() const;

    /// @brief 获取错误信息
    /// @return 错误描述
    std::string GetMessage() const;

    /// @brief 是否为空结果
    /// @return true 表示无音频内容
    bool IsEmpty() const;

    /// @brief 是否为句子结束（流式模式）
    /// @return true 表示当前句子合成完成
    bool IsSentenceEnd() const;

    // -------------------------------------------------------------------------
    // 性能指标
    // -------------------------------------------------------------------------

    /// @brief 获取采样率
    /// @return 采样率 (Hz)
    int GetSampleRate() const;

    /// @brief 获取音频时长
    /// @return 音频时长（毫秒）
    int GetDurationMs() const;

    /// @brief 获取处理时间
    /// @return 处理耗时（毫秒）
    int GetProcessingTimeMs() const;

    /// @brief 获取实时率 (RTF)
    /// @return 处理时间 / 音频时长
    float GetRTF() const;

    // -------------------------------------------------------------------------
    // 文件操作
    // -------------------------------------------------------------------------

    /// @brief 保存到文件
    /// @param file_path 文件路径
    /// @return 是否成功
    bool SaveToFile(const std::string& file_path) const;

private:
    friend class TtsEngine;
    friend class CallbackAdapter;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// TtsResultCallback - 回调接口
// =============================================================================

/**
 * @brief TTS 引擎回调接口（多态基类）
 *
 * 用户可继承此类并重写虚函数，以接收流式合成过程中的事件通知。
 *
 * ## 回调调用链（调用顺序）
 *
 * 流式合成的回调调用顺序如下：
 *
 * ```
 *   StreamingCall() / Start()
 *      │
 *      ▼
 *   OnOpen()              ← 合成会话开始
 *      │
 *      ▼
 *   [合成进行中] ────────► OnEvent()  ← 每完成一句话触发
 *      │                     │
 *      │                     └─ IsSentenceEnd()=true: 句子结束
 *      ▼
 *   OnComplete()          ← 合成正常完成
 *      │
 *      ▼
 *   OnClose()             ← 会话关闭
 * ```
 *
 * ## 错误处理
 *
 * 如果合成过程中发生错误，调用链为：
 * ```
 *   OnOpen() → ... → OnError() → OnClose()
 * ```
 *
 * ## 线程安全
 *
 * - 回调可能在引擎内部线程中被调用，实现时需注意线程安全
 * - 不要在回调中执行耗时操作，以免阻塞合成流程
 *
 * ## 使用示例
 *
 * ```cpp
 * class MyCallback : public TtsResultCallback {
 * public:
 *     void OnOpen() override {
 *         std::cout << "开始合成" << std::endl;
 *     }
 *
 *     void OnEvent(std::shared_ptr<TtsEngineResult> result) override {
 *         auto audio = result->GetAudioFloat();
 *         std::cout << "收到音频: " << audio.size() << " 样本" << std::endl;
 *         // 播放音频...
 *     }
 *
 *     void OnComplete() override {
 *         std::cout << "合成完成" << std::endl;
 *     }
 *
 *     void OnError(const std::string& message) override {
 *         std::cerr << "错误: " << message << std::endl;
 *     }
 *
 *     void OnClose() override {
 *         std::cout << "会话关闭" << std::endl;
 *     }
 * };
 * ```
 */
class TtsResultCallback {
public:
    virtual ~TtsResultCallback() = default;

    /// @brief 连接建立成功，合成会话开始
    virtual void OnOpen() {}

    /// @brief 收到合成结果（音频块）
    /// @param result 合成结果对象，包含音频数据
    /// @note 流式模式下每完成一句话触发一次
    virtual void OnEvent(std::shared_ptr<TtsEngineResult> result) {}

    /// @brief 合成任务正常完成
    virtual void OnComplete() {}

    /// @brief 发生错误
    /// @param message 错误描述
    virtual void OnError(const std::string& message) {}

    /// @brief 会话关闭
    /// @note 无论正常结束还是错误，最后都会调用此方法
    virtual void OnClose() {}
};

// =============================================================================
// TtsEngine - TTS 引擎
// =============================================================================

class TtsEngine {
public:
    // =========================================================================
    // DuplexStream - 双向流
    // =========================================================================

    /**
     * @brief 双向流接口
     *
     * 用于边输入文本边合成的场景。
     *
     * 使用示例:
     * ```cpp
     * auto stream = engine->StartDuplexStream(callback);
     * stream->SendText("第一句话。");
     * stream->SendText("第二句话。");
     * stream->Complete();  // 通知输入结束
     * ```
     */
    class DuplexStream {
    public:
        virtual ~DuplexStream() = default;

        /// @brief 发送文本进行合成
        /// @param text 要合成的文本
        virtual void SendText(const std::string& text) = 0;

        /// @brief 通知输入完成
        virtual void Complete() = 0;

        /// @brief 检查流是否活跃
        /// @return true 表示流仍在活跃状态
        virtual bool IsActive() const = 0;
    };

    // =========================================================================
    // 构造函数
    // =========================================================================

    /// @brief 构造 TTS 引擎（使用默认配置）
    /// @param backend 后端类型
    /// @param model_dir 模型目录路径，空则使用默认路径
    explicit TtsEngine(
            BackendType backend = BackendType::MATCHA_ZH,
            const std::string& model_dir = "");

    /// @brief 构造 TTS 引擎（使用配置结构体）
    /// @param config 配置对象
    explicit TtsEngine(const TtsConfig& config);

    /// @brief 析构函数
    virtual ~TtsEngine();

    // 禁止拷贝
    TtsEngine(const TtsEngine&) = delete;
    TtsEngine& operator=(const TtsEngine&) = delete;

    // =========================================================================
    // 非流式调用（阻塞）
    // =========================================================================

    /// @brief 合成文本（阻塞直到完成）
    /// @param text 要合成的文本
    /// @param config 可选的配置覆盖
    /// @return 合成结果，失败返回 nullptr
    std::shared_ptr<TtsEngineResult> Call(
        const std::string& text,
        const TtsConfig& config = TtsConfig());

    /// @brief 合成文本并保存到文件
    /// @param text 要合成的文本
    /// @param file_path 输出文件路径
    /// @return 是否成功
    bool CallToFile(const std::string& text, const std::string& file_path);

    // =========================================================================
    // 流式调用
    // =========================================================================

    /// @brief 流式合成（回调模式）
    /// @param text 要合成的文本
    /// @param callback 回调对象
    /// @param config 可选的配置覆盖
    void StreamingCall(
            const std::string& text,
            std::shared_ptr<TtsResultCallback> callback,
            const TtsConfig& config = TtsConfig());

    /// @brief 启动双向流
    /// @param callback 回调对象
    /// @param config 可选的配置覆盖
    /// @return 双向流对象
    std::shared_ptr<DuplexStream> StartDuplexStream(
        std::shared_ptr<TtsResultCallback> callback,
        const TtsConfig& config = TtsConfig());

    // =========================================================================
    // 动态配置
    // =========================================================================

    /// @brief 设置语速
    /// @param speed 语速倍率 (>1.0快, <1.0慢)
    void SetSpeed(float speed);

    /// @brief 设置说话人
    /// @param speaker_id 说话人ID
    void SetSpeaker(int speaker_id);

    /// @brief 设置音量
    /// @param volume 音量 [0, 100]
    void SetVolume(int volume);

    /// @brief 获取当前配置
    /// @return 配置对象
    TtsConfig GetConfig() const;

    // =========================================================================
    // 辅助方法
    // =========================================================================

    /// @brief 检查引擎是否已初始化
    /// @return true 表示已初始化
    bool IsInitialized() const;

    /// @brief 获取引擎类型名称
    /// @return 引擎名称
    std::string GetEngineName() const;

    /// @brief 获取后端类型
    /// @return 后端类型枚举
    BackendType GetBackendType() const;

    /// @brief 获取支持的说话人数量
    /// @return 说话人数量
    int GetNumSpeakers() const;

    /// @brief 获取输出采样率
    /// @return 采样率 (Hz)
    int GetSampleRate() const;

    /// @brief 获取最后一次请求 ID
    /// @return 请求 ID
    std::string GetLastRequestId() const;

private:
    friend class CallbackAdapter;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace Evo

#endif  // TTS_API_HPP
