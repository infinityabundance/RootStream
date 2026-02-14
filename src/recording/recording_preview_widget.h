#ifndef RECORDING_PREVIEW_WIDGET_H
#define RECORDING_PREVIEW_WIDGET_H

#include <QWidget>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QCheckBox>
#include <QSlider>

class RecordingPreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit RecordingPreviewWidget(QWidget *parent = nullptr);
    ~RecordingPreviewWidget();

    // Update preview with new frame
    void updateFrame(const uint8_t *frame_data, uint32_t width, uint32_t height, const char *format);
    
    // Enable/disable preview
    void setPreviewEnabled(bool enabled);
    bool isPreviewEnabled() const { return previewEnabled; }
    
    // Set preview quality (scale factor: 0.25 = 1/4 size, 1.0 = full size)
    void setPreviewQuality(float scale_factor);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onEnableToggled(bool checked);
    void onQualityChanged(int value);

private:
    void setupUI();
    QImage convertFrame(const uint8_t *frame_data, uint32_t width, uint32_t height, const char *format);
    
    QLabel *previewLabel;
    QCheckBox *enableCheckBox;
    QSlider *qualitySlider;
    
    QImage currentFrame;
    QPixmap scaledPixmap;
    
    bool previewEnabled;
    float scaleFactor;
    uint32_t frameCount;
    uint64_t lastUpdateTime;
};

#endif /* RECORDING_PREVIEW_WIDGET_H */
