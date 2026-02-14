#include "recording_preview_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QPaintEvent>

RecordingPreviewWidget::RecordingPreviewWidget(QWidget *parent)
    : QWidget(parent)
    , previewEnabled(false)
    , scaleFactor(0.5f)  // Default to half size for performance
    , frameCount(0)
    , lastUpdateTime(0)
{
    setupUI();
}

RecordingPreviewWidget::~RecordingPreviewWidget() {
}

void RecordingPreviewWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Preview display area
    previewLabel = new QLabel();
    previewLabel->setMinimumSize(320, 180);
    previewLabel->setMaximumSize(960, 540);
    previewLabel->setScaledContents(true);
    previewLabel->setStyleSheet("QLabel { background-color: black; border: 1px solid gray; }");
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setText("Preview Disabled");
    mainLayout->addWidget(previewLabel);
    
    // Controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    
    enableCheckBox = new QCheckBox("Enable Preview");
    enableCheckBox->setChecked(false);
    connect(enableCheckBox, &QCheckBox::toggled, this, &RecordingPreviewWidget::onEnableToggled);
    controlsLayout->addWidget(enableCheckBox);
    
    controlsLayout->addWidget(new QLabel("Quality:"));
    qualitySlider = new QSlider(Qt::Horizontal);
    qualitySlider->setMinimum(25);   // 0.25x scale
    qualitySlider->setMaximum(100);  // 1.0x scale
    qualitySlider->setValue(50);     // 0.5x default
    qualitySlider->setEnabled(false);
    connect(qualitySlider, &QSlider::valueChanged, this, &RecordingPreviewWidget::onQualityChanged);
    controlsLayout->addWidget(qualitySlider);
    
    controlsLayout->addStretch();
    
    mainLayout->addLayout(controlsLayout);
}

void RecordingPreviewWidget::onEnableToggled(bool checked) {
    previewEnabled = checked;
    qualitySlider->setEnabled(checked);
    
    if (!checked) {
        previewLabel->clear();
        previewLabel->setText("Preview Disabled");
        currentFrame = QImage();
    }
}

void RecordingPreviewWidget::onQualityChanged(int value) {
    scaleFactor = value / 100.0f;
}

void RecordingPreviewWidget::setPreviewEnabled(bool enabled) {
    previewEnabled = enabled;
    enableCheckBox->setChecked(enabled);
}

void RecordingPreviewWidget::setPreviewQuality(float scale_factor) {
    scaleFactor = qBound(0.25f, scale_factor, 1.0f);
    qualitySlider->setValue((int)(scaleFactor * 100));
}

QImage RecordingPreviewWidget::convertFrame(const uint8_t *frame_data, 
                                           uint32_t width, uint32_t height, 
                                           const char *format) {
    // Convert frame data to QImage based on pixel format
    QImage::Format img_format = QImage::Format_RGB888;
    
    if (strcmp(format, "rgb") == 0 || strcmp(format, "rgb24") == 0) {
        img_format = QImage::Format_RGB888;
    } else if (strcmp(format, "rgba") == 0 || strcmp(format, "rgba32") == 0) {
        img_format = QImage::Format_RGBA8888;
    } else if (strcmp(format, "bgr") == 0 || strcmp(format, "bgr24") == 0) {
        img_format = QImage::Format_BGR888;
    } else if (strcmp(format, "bgra") == 0) {
        // Qt doesn't have Format_BGRA8888, need to convert
        img_format = QImage::Format_ARGB32;
    }
    
    // Calculate bytes per line
    int bytes_per_pixel = 3;
    if (img_format == QImage::Format_RGBA8888 || img_format == QImage::Format_ARGB32) {
        bytes_per_pixel = 4;
    }
    int bytes_per_line = width * bytes_per_pixel;
    
    // Create QImage from frame data
    QImage image(frame_data, width, height, bytes_per_line, img_format);
    
    // Scale down if needed for performance
    if (scaleFactor < 1.0f) {
        int scaled_width = (int)(width * scaleFactor);
        int scaled_height = (int)(height * scaleFactor);
        image = image.scaled(scaled_width, scaled_height, 
                            Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
    
    return image.copy();  // Deep copy
}

void RecordingPreviewWidget::updateFrame(const uint8_t *frame_data, 
                                        uint32_t width, uint32_t height, 
                                        const char *format) {
    if (!previewEnabled || !frame_data) {
        return;
    }
    
    // Throttle updates to max 30 FPS for preview
    uint64_t current_time = QDateTime::currentMSecsSinceEpoch();
    if (current_time - lastUpdateTime < 33) {  // ~30 FPS
        return;
    }
    lastUpdateTime = current_time;
    
    // Convert frame to QImage
    currentFrame = convertFrame(frame_data, width, height, format);
    
    if (!currentFrame.isNull()) {
        // Scale to fit label while maintaining aspect ratio
        scaledPixmap = QPixmap::fromImage(currentFrame).scaled(
            previewLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        
        previewLabel->setPixmap(scaledPixmap);
        frameCount++;
    }
}

void RecordingPreviewWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
}

void RecordingPreviewWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    
    // Update scaled pixmap when widget is resized
    if (!currentFrame.isNull()) {
        scaledPixmap = QPixmap::fromImage(currentFrame).scaled(
            previewLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        previewLabel->setPixmap(scaledPixmap);
    }
}
