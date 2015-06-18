#ifndef IMAGECANVAS_H
#define IMAGECANVAS_H

#include <QImage>
#include <QtWidgets>
#include <QGLWidget>
#include <QOpenGLFunctions>

QT_FORWARD_DECLARE_CLASS(QGLShaderProgram)

class ImageCanvas : public QGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit ImageCanvas(QWidget *parent = 0, QGLWidget *shareWidget = 0);
    ~ImageCanvas();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void setClearColor(const QColor &color);
    void updateImage(float *ptr, int w, int h, int ch);
    void updateTexData(const float* ptr, unsigned w, unsigned h);

signals:
    void clicked();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    void makeObject();

    QColor clearColor;
    QPoint lastPos;
    GLuint texture;
    GLint  mCurrFormat;
    QVector<QVector3D> vertices;
    QVector<QVector2D> texCoords;
    QGLShaderProgram *program;
};

#endif // IMAGECANVAS_H
