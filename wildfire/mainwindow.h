#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "ImageCanvas.h"

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

private:
    Ui::MainWindow *ui;

    ImageCanvas *mRenderCanvas;
    QImage *mCurrentImage;
    unsigned mImageWidth;
    unsigned mImageHeight;
};

#endif // MAINWINDOW_H
