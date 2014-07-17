#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mRenderCanvas(0),
    mCurrentImage(0)
{
    ui->setupUi(this);
    setWindowTitle(tr("WildFire Image Editor"));
    // setup render window
    mRenderCanvas = new ImageCanvas();
    this->setCentralWidget(mRenderCanvas);
    connect(ui->actionOpen,SIGNAL(triggered()),this,SLOT(loadImage()));
    connect(ui->actionSave,SIGNAL(triggered()),this,SLOT(saveImage()));
    connect(ui->actionExit,SIGNAL(triggered()),QApplication::instance(),SLOT(quit()));
}

void MainWindow::loadImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open Image"),"",tr("*.png *.jpg *.bmp"));
    if(!fileName.isEmpty())
    {
        QImage imageHandle(fileName);
        if (imageHandle.isNull()) {
            QMessageBox::information(this, tr("Image Viewer"), tr("Cannot load %1.").arg(fileName));
            return;
        }
        mImageWidth = imageHandle.width();
        mImageHeight= imageHandle.height();
        mRenderCanvas->updateImage(imageHandle);
        mCurrentImage = new QImage(imageHandle);
    }
}

void MainWindow::saveImage()
{
    QString fileName = QFileDialog::getSaveFileName(this,tr("Save Image"),"",tr("*.png *.jpg *.bmp"));
    if(!fileName.isEmpty())
    {
        mCurrentImage->save(fileName);
    }
}


MainWindow::~MainWindow()
{
    delete ui;
    delete mRenderCanvas;
    if (mCurrentImage) delete mCurrentImage;
}
