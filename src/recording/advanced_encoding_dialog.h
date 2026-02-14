#ifndef ADVANCED_ENCODING_DIALOG_H
#define ADVANCED_ENCODING_DIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>

extern "C" {
#include "../recording_types.h"
}

struct EncodingOptions {
    // Video options
    VideoCodec codec;
    uint32_t bitrate_kbps;
    uint32_t fps;
    uint32_t width;
    uint32_t height;
    int quality_crf;  // For H.264/AV1 CRF mode (-1 = use bitrate)
    
    // Codec-specific options
    QString h264_preset;  // veryfast, fast, medium, slow, veryslow
    int vp9_cpu_used;     // 0-5
    int av1_cpu_used;     // 0-8
    
    // Advanced options
    int gop_size;         // Keyframe interval
    int max_b_frames;     // Maximum B-frames
    bool use_two_pass;    // Two-pass encoding
    
    // Audio options
    AudioCodec audio_codec;
    uint32_t audio_bitrate_kbps;
    uint32_t audio_sample_rate;
    uint8_t audio_channels;
    
    // Container options
    ContainerFormat container;
    
    // HDR options (future)
    bool enable_hdr;
    QString hdr_format;   // "HDR10", "HLG", etc.
};

class AdvancedEncodingDialog : public QDialog {
    Q_OBJECT

public:
    explicit AdvancedEncodingDialog(QWidget *parent = nullptr);
    ~AdvancedEncodingDialog();
    
    // Get/set encoding options
    EncodingOptions getOptions() const;
    void setOptions(const EncodingOptions &options);
    
    // Load/save presets
    void loadPreset(RecordingPreset preset);
    void saveAsPreset(const QString &name);

private slots:
    void onCodecChanged(int index);
    void onQualityModeChanged(int state);
    void onResetClicked();
    void onPresetLoaded(int index);

private:
    void setupUI();
    void updateCodecSpecificOptions();
    void applyPreset(RecordingPreset preset);
    
    // Video codec selection
    QComboBox *codecComboBox;
    QSpinBox *bitrateSpinBox;
    QSpinBox *fpsSpinBox;
    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
    
    // Quality mode
    QCheckBox *crfModeCheckBox;
    QSpinBox *crfSpinBox;
    
    // Codec-specific options
    QGroupBox *h264GroupBox;
    QComboBox *h264PresetComboBox;
    
    QGroupBox *vp9GroupBox;
    QSpinBox *vp9CpuUsedSpinBox;
    
    QGroupBox *av1GroupBox;
    QSpinBox *av1CpuUsedSpinBox;
    
    // Advanced video options
    QSpinBox *gopSizeSpinBox;
    QSpinBox *maxBFramesSpinBox;
    QCheckBox *twoPassCheckBox;
    
    // Audio options
    QComboBox *audioCodecComboBox;
    QSpinBox *audioBitrateSpinBox;
    QComboBox *audioSampleRateComboBox;
    QComboBox *audioChannelsComboBox;
    
    // Container options
    QComboBox *containerComboBox;
    
    // HDR options (future)
    QCheckBox *hdrCheckBox;
    QComboBox *hdrFormatComboBox;
    
    // Preset selection
    QComboBox *presetComboBox;
};

#endif /* ADVANCED_ENCODING_DIALOG_H */
