#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ImageCanvas.h"
#include "imageEdit.hpp"

Q_DECLARE_METATYPE(af::array)

float convertRange(float value,  float dst_max, float dst_min, float src_max, float src_min);

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void loadImage(void);
    void saveImage(void);
    void contrastChanged(int);
    void brightnessChanged(int);
    void usmRadiusChanged(int);
    void usmChanged(int);
    void zoomParamsChanged(void);
    void zoomReset(void);
    void setBackgroundImageForBlend(void);
    void setForegroudImageForBlend(void);
    void setMaskImageForBlend(void);
    void showBackground(void);
    void showForeground(void);
    void showMask(void);
    void showBlendedImage(void);

private:
    Ui::MainWindow *ui;

    ImageCanvas *mRenderCanvas;
    af::array mCurrentImage;
    float* mImageDataRawPtr;
    unsigned mImageWidth;
    unsigned mImageHeight;
    int arrayRegisterId;
    /* processing algorithm transient variables */
    int mCurrUSMRadius;
    float mCurrUSMSharpness;
    af::array mBg4Blend;
    af::array mFg4Blend;
    af::array mMsk4Blend;
};

#endif // MAINWINDOW_H
