#include "advanced_encoding_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QDialogButtonBox>

AdvancedEncodingDialog::AdvancedEncodingDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Advanced Encoding Options");
    setMinimumWidth(600);
    setupUI();
    
    // Load balanced preset by default
    loadPreset(PRESET_BALANCED);
}

AdvancedEncodingDialog::~AdvancedEncodingDialog() {
}

void AdvancedEncodingDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Preset selector at top
    QHBoxLayout *presetLayout = new QHBoxLayout();
    presetLayout->addWidget(new QLabel("Load Preset:"));
    presetComboBox = new QComboBox();
    presetComboBox->addItem("Fast (H.264, 20 Mbps)", PRESET_FAST);
    presetComboBox->addItem("Balanced (H.264, 8-10 Mbps)", PRESET_BALANCED);
    presetComboBox->addItem("High Quality (VP9, 5-8 Mbps)", PRESET_HIGH_QUALITY);
    presetComboBox->addItem("Archival (AV1, 2-4 Mbps)", PRESET_ARCHIVAL);
    connect(presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AdvancedEncodingDialog::onPresetLoaded);
    presetLayout->addWidget(presetComboBox);
    presetLayout->addStretch();
    mainLayout->addLayout(presetLayout);
    
    // Tabbed interface
    QTabWidget *tabWidget = new QTabWidget();
    
    // Video tab
    QWidget *videoTab = new QWidget();
    QFormLayout *videoLayout = new QFormLayout(videoTab);
    
    codecComboBox = new QComboBox();
    codecComboBox->addItem("H.264 (Fast, Universal)", VIDEO_CODEC_H264);
    codecComboBox->addItem("VP9 (Better Compression)", VIDEO_CODEC_VP9);
    codecComboBox->addItem("AV1 (Best Compression)", VIDEO_CODEC_AV1);
    connect(codecComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AdvancedEncodingDialog::onCodecChanged);
    videoLayout->addRow("Video Codec:", codecComboBox);
    
    // Resolution
    QHBoxLayout *resLayout = new QHBoxLayout();
    widthSpinBox = new QSpinBox();
    widthSpinBox->setRange(640, 3840);
    widthSpinBox->setValue(1920);
    widthSpinBox->setSuffix(" px");
    resLayout->addWidget(widthSpinBox);
    resLayout->addWidget(new QLabel("×"));
    heightSpinBox = new QSpinBox();
    heightSpinBox->setRange(480, 2160);
    heightSpinBox->setValue(1080);
    heightSpinBox->setSuffix(" px");
    resLayout->addWidget(heightSpinBox);
    videoLayout->addRow("Resolution:", resLayout);
    
    fpsSpinBox = new QSpinBox();
    fpsSpinBox->setRange(15, 120);
    fpsSpinBox->setValue(60);
    fpsSpinBox->setSuffix(" fps");
    videoLayout->addRow("Frame Rate:", fpsSpinBox);
    
    // Quality mode
    crfModeCheckBox = new QCheckBox("Use CRF (Constant Quality) Mode");
    connect(crfModeCheckBox, &QCheckBox::stateChanged,
            this, &AdvancedEncodingDialog::onQualityModeChanged);
    videoLayout->addRow(crfModeCheckBox);
    
    bitrateSpinBox = new QSpinBox();
    bitrateSpinBox->setRange(1000, 50000);
    bitrateSpinBox->setValue(8000);
    bitrateSpinBox->setSuffix(" kbps");
    videoLayout->addRow("Bitrate:", bitrateSpinBox);
    
    crfSpinBox = new QSpinBox();
    crfSpinBox->setRange(0, 51);
    crfSpinBox->setValue(23);
    crfSpinBox->setEnabled(false);
    videoLayout->addRow("CRF Quality:", crfSpinBox);
    
    // Codec-specific options
    // H.264 options
    h264GroupBox = new QGroupBox("H.264 Options");
    QFormLayout *h264Layout = new QFormLayout(h264GroupBox);
    h264PresetComboBox = new QComboBox();
    h264PresetComboBox->addItem("Ultra Fast", "ultrafast");
    h264PresetComboBox->addItem("Very Fast", "veryfast");
    h264PresetComboBox->addItem("Fast", "fast");
    h264PresetComboBox->addItem("Medium", "medium");
    h264PresetComboBox->addItem("Slow", "slow");
    h264PresetComboBox->addItem("Very Slow", "veryslow");
    h264PresetComboBox->setCurrentIndex(2);  // fast
    h264Layout->addRow("Preset:", h264PresetComboBox);
    videoLayout->addRow(h264GroupBox);
    
    // VP9 options
    vp9GroupBox = new QGroupBox("VP9 Options");
    QFormLayout *vp9Layout = new QFormLayout(vp9GroupBox);
    vp9CpuUsedSpinBox = new QSpinBox();
    vp9CpuUsedSpinBox->setRange(0, 5);
    vp9CpuUsedSpinBox->setValue(2);
    vp9Layout->addRow("CPU Used (0=slow, 5=fast):", vp9CpuUsedSpinBox);
    vp9GroupBox->setVisible(false);
    videoLayout->addRow(vp9GroupBox);
    
    // AV1 options
    av1GroupBox = new QGroupBox("AV1 Options");
    QFormLayout *av1Layout = new QFormLayout(av1GroupBox);
    av1CpuUsedSpinBox = new QSpinBox();
    av1CpuUsedSpinBox->setRange(0, 8);
    av1CpuUsedSpinBox->setValue(4);
    av1Layout->addRow("CPU Used (0=slow, 8=fast):", av1CpuUsedSpinBox);
    av1GroupBox->setVisible(false);
    videoLayout->addRow(av1GroupBox);
    
    // Advanced options
    QGroupBox *advancedGroup = new QGroupBox("Advanced Video Options");
    QFormLayout *advancedLayout = new QFormLayout(advancedGroup);
    
    gopSizeSpinBox = new QSpinBox();
    gopSizeSpinBox->setRange(10, 600);
    gopSizeSpinBox->setValue(120);
    gopSizeSpinBox->setSuffix(" frames");
    advancedLayout->addRow("Keyframe Interval:", gopSizeSpinBox);
    
    maxBFramesSpinBox = new QSpinBox();
    maxBFramesSpinBox->setRange(0, 4);
    maxBFramesSpinBox->setValue(0);
    advancedLayout->addRow("Max B-Frames:", maxBFramesSpinBox);
    
    twoPassCheckBox = new QCheckBox("Enable Two-Pass Encoding");
    advancedLayout->addRow(twoPassCheckBox);
    
    videoLayout->addRow(advancedGroup);
    
    tabWidget->addTab(videoTab, "Video");
    
    // Audio tab
    QWidget *audioTab = new QWidget();
    QFormLayout *audioLayout = new QFormLayout(audioTab);
    
    audioCodecComboBox = new QComboBox();
    audioCodecComboBox->addItem("Opus (Recommended)", AUDIO_CODEC_OPUS);
    audioCodecComboBox->addItem("AAC (Compatible)", AUDIO_CODEC_AAC);
    audioLayout->addRow("Audio Codec:", audioCodecComboBox);
    
    audioBitrateSpinBox = new QSpinBox();
    audioBitrateSpinBox->setRange(64, 320);
    audioBitrateSpinBox->setValue(128);
    audioBitrateSpinBox->setSuffix(" kbps");
    audioLayout->addRow("Audio Bitrate:", audioBitrateSpinBox);
    
    audioSampleRateComboBox = new QComboBox();
    audioSampleRateComboBox->addItem("44.1 kHz", 44100);
    audioSampleRateComboBox->addItem("48 kHz", 48000);
    audioSampleRateComboBox->setCurrentIndex(1);
    audioLayout->addRow("Sample Rate:", audioSampleRateComboBox);
    
    audioChannelsComboBox = new QComboBox();
    audioChannelsComboBox->addItem("Mono", 1);
    audioChannelsComboBox->addItem("Stereo", 2);
    audioChannelsComboBox->setCurrentIndex(1);
    audioLayout->addRow("Channels:", audioChannelsComboBox);
    
    tabWidget->addTab(audioTab, "Audio");
    
    // Container tab
    QWidget *containerTab = new QWidget();
    QFormLayout *containerLayout = new QFormLayout(containerTab);
    
    containerComboBox = new QComboBox();
    containerComboBox->addItem("MP4 (Universal)", CONTAINER_MP4);
    containerComboBox->addItem("Matroska/MKV (Advanced)", CONTAINER_MATROSKA);
    containerLayout->addRow("Container Format:", containerComboBox);
    
    tabWidget->addTab(containerTab, "Container");
    
    // HDR tab (future)
    QWidget *hdrTab = new QWidget();
    QFormLayout *hdrLayout = new QFormLayout(hdrTab);
    
    hdrCheckBox = new QCheckBox("Enable HDR");
    hdrCheckBox->setEnabled(false);  // Future feature
    hdrLayout->addRow(hdrCheckBox);
    
    hdrFormatComboBox = new QComboBox();
    hdrFormatComboBox->addItem("HDR10", "hdr10");
    hdrFormatComboBox->addItem("HLG", "hlg");
    hdrFormatComboBox->setEnabled(false);
    hdrLayout->addRow("HDR Format:", hdrFormatComboBox);
    
    QLabel *hdrNote = new QLabel("<i>HDR support coming in future update</i>");
    hdrLayout->addRow(hdrNote);
    
    tabWidget->addTab(hdrTab, "HDR");
    
    mainLayout->addWidget(tabWidget);
    
    // Button box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
            this, &AdvancedEncodingDialog::onResetClicked);
    mainLayout->addWidget(buttonBox);
}

void AdvancedEncodingDialog::onCodecChanged(int index) {
    updateCodecSpecificOptions();
}

void AdvancedEncodingDialog::onQualityModeChanged(int state) {
    bool crfMode = (state == Qt::Checked);
    bitrateSpinBox->setEnabled(!crfMode);
    crfSpinBox->setEnabled(crfMode);
}

void AdvancedEncodingDialog::onResetClicked() {
    loadPreset(PRESET_BALANCED);
}

void AdvancedEncodingDialog::onPresetLoaded(int index) {
    RecordingPreset preset = static_cast<RecordingPreset>(
        presetComboBox->itemData(index).toInt()
    );
    loadPreset(preset);
}

void AdvancedEncodingDialog::updateCodecSpecificOptions() {
    VideoCodec codec = static_cast<VideoCodec>(codecComboBox->currentData().toInt());
    
    h264GroupBox->setVisible(codec == VIDEO_CODEC_H264);
    vp9GroupBox->setVisible(codec == VIDEO_CODEC_VP9);
    av1GroupBox->setVisible(codec == VIDEO_CODEC_AV1);
}

void AdvancedEncodingDialog::loadPreset(RecordingPreset preset) {
    switch (preset) {
        case PRESET_FAST:
            codecComboBox->setCurrentIndex(0);  // H.264
            bitrateSpinBox->setValue(20000);
            h264PresetComboBox->setCurrentText("Very Fast");
            crfModeCheckBox->setChecked(false);
            twoPassCheckBox->setChecked(false);
            containerComboBox->setCurrentIndex(0);  // MP4
            break;
            
        case PRESET_BALANCED:
            codecComboBox->setCurrentIndex(0);  // H.264
            bitrateSpinBox->setValue(8000);
            h264PresetComboBox->setCurrentText("Medium");
            crfModeCheckBox->setChecked(false);
            twoPassCheckBox->setChecked(false);
            containerComboBox->setCurrentIndex(0);  // MP4
            break;
            
        case PRESET_HIGH_QUALITY:
            codecComboBox->setCurrentIndex(1);  // VP9
            bitrateSpinBox->setValue(6000);
            vp9CpuUsedSpinBox->setValue(2);
            crfModeCheckBox->setChecked(false);
            twoPassCheckBox->setChecked(false);
            containerComboBox->setCurrentIndex(1);  // MKV
            break;
            
        case PRESET_ARCHIVAL:
            codecComboBox->setCurrentIndex(2);  // AV1
            bitrateSpinBox->setValue(3000);
            av1CpuUsedSpinBox->setValue(4);
            crfModeCheckBox->setChecked(true);
            crfSpinBox->setValue(30);
            twoPassCheckBox->setChecked(false);
            containerComboBox->setCurrentIndex(1);  // MKV
            break;
    }
    
    updateCodecSpecificOptions();
}

EncodingOptions AdvancedEncodingDialog::getOptions() const {
    EncodingOptions options;
    
    options.codec = static_cast<VideoCodec>(codecComboBox->currentData().toInt());
    options.bitrate_kbps = bitrateSpinBox->value();
    options.fps = fpsSpinBox->value();
    options.width = widthSpinBox->value();
    options.height = heightSpinBox->value();
    options.quality_crf = crfModeCheckBox->isChecked() ? crfSpinBox->value() : -1;
    
    options.h264_preset = h264PresetComboBox->currentData().toString();
    options.vp9_cpu_used = vp9CpuUsedSpinBox->value();
    options.av1_cpu_used = av1CpuUsedSpinBox->value();
    
    options.gop_size = gopSizeSpinBox->value();
    options.max_b_frames = maxBFramesSpinBox->value();
    options.use_two_pass = twoPassCheckBox->isChecked();
    
    options.audio_codec = static_cast<AudioCodec>(audioCodecComboBox->currentData().toInt());
    options.audio_bitrate_kbps = audioBitrateSpinBox->value();
    options.audio_sample_rate = audioSampleRateComboBox->currentData().toUInt();
    options.audio_channels = audioChannelsComboBox->currentData().toUInt();
    
    options.container = static_cast<ContainerFormat>(containerComboBox->currentData().toInt());
    
    options.enable_hdr = hdrCheckBox->isChecked();
    options.hdr_format = hdrFormatComboBox->currentData().toString();
    
    return options;
}

void AdvancedEncodingDialog::setOptions(const EncodingOptions &options) {
    // Set all UI elements based on options
    codecComboBox->setCurrentIndex(options.codec);
    bitrateSpinBox->setValue(options.bitrate_kbps);
    fpsSpinBox->setValue(options.fps);
    widthSpinBox->setValue(options.width);
    heightSpinBox->setValue(options.height);
    
    if (options.quality_crf >= 0) {
        crfModeCheckBox->setChecked(true);
        crfSpinBox->setValue(options.quality_crf);
    } else {
        crfModeCheckBox->setChecked(false);
    }
    
    // Find and set h264 preset
    int h264Index = h264PresetComboBox->findData(options.h264_preset);
    if (h264Index >= 0) {
        h264PresetComboBox->setCurrentIndex(h264Index);
    }
    
    vp9CpuUsedSpinBox->setValue(options.vp9_cpu_used);
    av1CpuUsedSpinBox->setValue(options.av1_cpu_used);
    
    gopSizeSpinBox->setValue(options.gop_size);
    maxBFramesSpinBox->setValue(options.max_b_frames);
    twoPassCheckBox->setChecked(options.use_two_pass);
    
    audioCodecComboBox->setCurrentIndex(options.audio_codec);
    audioBitrateSpinBox->setValue(options.audio_bitrate_kbps);
    
    containerComboBox->setCurrentIndex(options.container);
    
    hdrCheckBox->setChecked(options.enable_hdr);
    
    updateCodecSpecificOptions();
}
