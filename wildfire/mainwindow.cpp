#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>

const float UI_CONTRAST_SLIDER_MIN = 0;
const float UI_CONTRAST_SLIDER_MAX = 99;
const float CONTRAST_ALGO_MIN =  -1.0f;
const float CONTRAST_ALGO_MAX =  1.0f;

const float UI_BRIGHTNESS_SLIDER_MIN = 0;
const float UI_BRIGHTNESS_SLIDER_MAX = 99;
const float BRIGHTNESS_ALGO_MIN =  0.0f;
const float BRIGHTNESS_ALGO_MAX =  1.0f;

const float UI_USMSHARP_SLIDER_MIN = 0;
const float UI_USMSHARP_SLIDER_MAX = 99;
const float USMSHARP_ALGO_MIN =  0.0f;
const float USMSHARP_ALGO_MAX =  2.0f;

float convertRange(float value,  float dst_max, float dst_min, float src_max, float src_min)
{
    return dst_min + (dst_max - dst_min)*((value - src_min)/(src_max-src_min));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      mRenderCanvas(0), mImageDataRawPtr(0), mCurrUSMRadius(1),
      mCurrUSMSharpness(0)
{
    arrayRegisterId = qRegisterMetaType<af::array>();
    ui->setupUi(this);
    setWindowTitle(tr("WildFire Image Editor"));
    removeToolBar(ui->mainToolBar);
    ui->zoomXLineEdit->setValidator(new QIntValidator());
    ui->zoomYLineEdit->setValidator(new QIntValidator());
    ui->zoomWidthLineEdit->setValidator(new QIntValidator());
    ui->zoomHeightLineEdit->setValidator(new QIntValidator());
    // setup render window
    mRenderCanvas = new ImageCanvas();
    this->setCentralWidget(mRenderCanvas);
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(loadImage()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveImage()));
    connect(ui->actionExit, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));
    connect(ui->contrastSlider, SIGNAL(valueChanged(int)), this, SLOT(contrastChanged(int)));
    connect(ui->brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(brightnessChanged(int)));
    connect(ui->usmRadiusSlider, SIGNAL(valueChanged(int)), this, SLOT(usmRadiusChanged(int)));
    connect(ui->usmSharpSlider, SIGNAL(valueChanged(int)), this, SLOT(usmChanged(int)));
    connect(ui->zoomPushButton, SIGNAL(clicked()), this, SLOT(zoomParamsChanged()));
    connect(ui->zoomResetPushButton, SIGNAL(clicked()), this, SLOT(zoomReset()));
    connect(ui->blendBgPushButton, SIGNAL(clicked()), this, SLOT(setBackgroundImageForBlend()));
    connect(ui->blendFgPushButton, SIGNAL(clicked()), this, SLOT(setForegroudImageForBlend()));
    connect(ui->blendMskPushButton, SIGNAL(clicked()), this, SLOT(setMaskImageForBlend()));
    connect(ui->blendBackRadioButton, SIGNAL(clicked()), this, SLOT(showBackground()));
    connect(ui->blendFrontRadioButton, SIGNAL(clicked()), this, SLOT(showForeground()));
    connect(ui->blendMskRadioButton, SIGNAL(clicked()), this, SLOT(showMask()));
    connect(ui->blendRadioButton, SIGNAL(clicked()), this, SLOT(showBlendedImage()));
    af::setDevice(0);
    af::info();
}

void MainWindow::loadImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open Image"),"",tr("*.png *.jpg *.bmp"));
    if(!fileName.isEmpty()) {
        af::deviceGC();
        mCurrentImage = af::loadImage(fileName.toStdString().c_str(), true);
        mImageWidth = mCurrentImage.dims(1);
        mImageHeight= mCurrentImage.dims(0);
        /* cleanup old memory allocation associated with earlier image */
        if (mImageDataRawPtr)
            delete[] mImageDataRawPtr;
        /* allocate memory with new loaded image */
        mImageDataRawPtr = new float[mCurrentImage.elements()];
        /* update host memory with device memory */
        af::array temp = af::reorder(mCurrentImage, 2, 1, 0)/255.0f;
        temp.host((void*)mImageDataRawPtr);
        mRenderCanvas->updateImage(mImageDataRawPtr,
                                   mImageWidth,
                                   mImageHeight,
                                   mCurrentImage.dims(2));
    }
}

void MainWindow::saveImage()
{
    QString fileName = QFileDialog::getSaveFileName(this,tr("Save Image"),"",tr("*.png *.jpg *.bmp"));
    if(!fileName.isEmpty()) {
        af::saveImage(fileName.toStdString().c_str(), mCurrentImage);
    }
}

void MainWindow::contrastChanged(int value)
{
    float param = convertRange(value, CONTRAST_ALGO_MAX, CONTRAST_ALGO_MIN,
                               UI_CONTRAST_SLIDER_MAX, UI_CONTRAST_SLIDER_MIN);
    af::array slices = changeContrast(mCurrentImage, param);
    af::array interleaved = af::reorder(slices, 2, 1, 0)/255.0f;
    interleaved.host((void*)mImageDataRawPtr);
    mRenderCanvas->updateTexData(mImageDataRawPtr, mImageWidth, mImageHeight);
    mRenderCanvas->updateGL();
}

void MainWindow::brightnessChanged(int value)
{
    float param = convertRange(value, BRIGHTNESS_ALGO_MAX, BRIGHTNESS_ALGO_MIN,
                               UI_BRIGHTNESS_SLIDER_MAX, UI_BRIGHTNESS_SLIDER_MIN);
    af::array slices = changeBrightness(mCurrentImage, param);
    af::array interleaved = af::reorder(slices, 2, 1, 0)/255.0f;
    interleaved.host((void*)mImageDataRawPtr);
    mRenderCanvas->updateTexData(mImageDataRawPtr, mImageWidth, mImageHeight);
    mRenderCanvas->updateGL();
}

void MainWindow::usmRadiusChanged(int value)
{
    mCurrUSMRadius = value;
    af::array slices = usm(mCurrentImage, mCurrUSMRadius, mCurrUSMSharpness);
    af::array interleaved = af::reorder(slices, 2, 1, 0)/255.0f;
    interleaved.host((void*)mImageDataRawPtr);
    mRenderCanvas->updateTexData(mImageDataRawPtr, mImageWidth, mImageHeight);
    mRenderCanvas->updateGL();
}

void MainWindow::usmChanged(int value)
{
    mCurrUSMSharpness = convertRange(value, USMSHARP_ALGO_MAX, USMSHARP_ALGO_MIN,
                               UI_USMSHARP_SLIDER_MAX, UI_USMSHARP_SLIDER_MIN);
    af::array slices = usm(mCurrentImage, mCurrUSMRadius, mCurrUSMSharpness);
    af::array interleaved = af::reorder(slices, 2, 1, 0)/255.0f;
    interleaved.host((void*)mImageDataRawPtr);
    mRenderCanvas->updateTexData(mImageDataRawPtr, mImageWidth, mImageHeight);
    mRenderCanvas->updateGL();
}

void MainWindow::zoomParamsChanged()
{
    int X = ui->zoomXLineEdit->text().toInt();
    int Y = ui->zoomYLineEdit->text().toInt();
    int W = ui->zoomWidthLineEdit->text().toInt();
    int H = ui->zoomHeightLineEdit->text().toInt();
    af::array slices = digZoom(mCurrentImage, X, Y, W, H);
    af::array interleaved = af::reorder(slices, 2, 1, 0)/255.0f;
    interleaved.host((void*)mImageDataRawPtr);
    mRenderCanvas->updateTexData(mImageDataRawPtr, mImageWidth, mImageHeight);
    mRenderCanvas->updateGL();
}

void MainWindow::zoomReset()
{
    af::array interleaved = af::reorder(mCurrentImage, 2, 1, 0)/255.0f;
    interleaved.host((void*)mImageDataRawPtr);
    mRenderCanvas->updateTexData(mImageDataRawPtr, mImageWidth, mImageHeight);
    mRenderCanvas->updateGL();
}

void MainWindow::setBackgroundImageForBlend()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open Image"),"",tr("*.png *.jpg *.bmp"));
    if(!fileName.isEmpty()) {
        af::deviceGC();
        mBg4Blend = af::loadImage(fileName.toStdString().c_str(), true);
        ui->blendBackLineEdit->setText(fileName);
    }
}

void MainWindow::setForegroudImageForBlend()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open Image"),"",tr("*.png *.jpg *.bmp"));
    if(!fileName.isEmpty()) {
        af::deviceGC();
        mFg4Blend = af::loadImage(fileName.toStdString().c_str(), true);
        ui->blendFrontLineEdit->setText(fileName);
    }
}

void MainWindow::setMaskImageForBlend()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open Image"),"",tr("*.png *.jpg *.bmp"));
    if(!fileName.isEmpty()) {
        af::deviceGC();
        mMsk4Blend = af::loadImage(fileName.toStdString().c_str(), false);
        ui->blendMaskLineEdit->setText(fileName);
    }
}

void MainWindow::showBackground()
{
    if (!mBg4Blend.isempty()) {
        /* cleanup old memory allocation associated with earlier image */
        if (mImageDataRawPtr)
            delete[] mImageDataRawPtr;
        /* allocate memory with new loaded image */
        mImageDataRawPtr = new float[mBg4Blend.elements()];
        /* update host memory with device memory */
        af::array temp = af::reorder(mBg4Blend, 2, 1, 0)/255.0f;
        temp.host((void*)mImageDataRawPtr);
        mRenderCanvas->updateImage(mImageDataRawPtr,
                                   mBg4Blend.dims(1),
                                   mBg4Blend.dims(0),
                                   mBg4Blend.dims(2));
        mRenderCanvas->updateGL();
    }
}

void MainWindow::showForeground()
{
    if (!mFg4Blend.isempty()) {
        /* cleanup old memory allocation associated with earlier image */
        if (mImageDataRawPtr)
            delete[] mImageDataRawPtr;
        /* allocate memory with new loaded image */
        mImageDataRawPtr = new float[mFg4Blend.elements()];
        /* update host memory with device memory */
        af::array temp = af::reorder(mFg4Blend, 2, 1, 0)/255.0f;
        temp.host((void*)mImageDataRawPtr);
        mRenderCanvas->updateImage(mImageDataRawPtr,
                                   mFg4Blend.dims(1),
                                   mFg4Blend.dims(0),
                                   mFg4Blend.dims(2));
        mRenderCanvas->updateGL();
    }
}

void MainWindow::showMask()
{
    if (!mMsk4Blend.isempty()) {
        /* cleanup old memory allocation associated with earlier image */
        if (mImageDataRawPtr)
            delete[] mImageDataRawPtr;
        /* allocate memory with new loaded image */
        mImageDataRawPtr = new float[mMsk4Blend.elements()];
        /* update host memory with device memory */
        af::array temp = af::reorder(mMsk4Blend, 1, 0)/255.0f;
        temp.host((void*)mImageDataRawPtr);
        mRenderCanvas->updateImage(mImageDataRawPtr,
                                   mMsk4Blend.dims(1),
                                   mMsk4Blend.dims(0),
                                   mMsk4Blend.dims(2));
        mRenderCanvas->updateGL();
    }
}

void MainWindow::showBlendedImage()
{
    bool isSameWidth = mBg4Blend.dims(0)== mFg4Blend.dims(0) && mBg4Blend.dims(0)== mMsk4Blend.dims(0);
    bool isSameHeight= mBg4Blend.dims(1)== mFg4Blend.dims(1) && mBg4Blend.dims(1)== mMsk4Blend.dims(1);
    bool isSameFormat= mBg4Blend.dims(2)== mFg4Blend.dims(2);
    bool isMaskGrayScale = mMsk4Blend.dims(2)==1;

    if (isSameWidth && isSameHeight && isSameFormat && isMaskGrayScale) {
        /* cleanup old memory allocation associated with earlier image */
        if (mImageDataRawPtr)
            delete[] mImageDataRawPtr;
        /* allocate memory with new loaded image */
        mImageDataRawPtr = new float[mBg4Blend.elements()];
        /* update host memory with device memory */
        af::array slices = alphaBlend(mFg4Blend, mBg4Blend, mMsk4Blend);
        af::array temp = af::reorder(slices, 2, 1, 0)/255.0f;
        temp.host((void*)mImageDataRawPtr);
        mRenderCanvas->updateImage(mImageDataRawPtr,
                                   mBg4Blend.dims(1),
                                   mBg4Blend.dims(0),
                                   mBg4Blend.dims(2));
        mRenderCanvas->updateGL();
    } else {
        QMessageBox::warning(this, "Invalid Input Warning", "Dimensions of the background, foreground and mask images do not match please check.");
    }
}

MainWindow::~MainWindow()
{
    af::deviceGC();
    delete[] mImageDataRawPtr;
    delete mRenderCanvas;
    delete ui;
}
