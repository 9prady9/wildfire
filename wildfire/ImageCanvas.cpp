#include "ImageCanvas.h"

#include <QGLShader>

ImageCanvas::ImageCanvas(QWidget *parent, QGLWidget *shareWidget)
    : QGLWidget(parent, shareWidget)
{
    clearColor = QColor(128,128,128);
    program = 0;
    texture = 0;
}

ImageCanvas::~ImageCanvas()
{
    vertices.clear();
    texCoords.clear();
    glDeleteTextures(1, &texture);
    delete program;
}

QSize ImageCanvas::minimumSizeHint() const
{
    return QSize(320, 240);
}

QSize ImageCanvas::sizeHint() const
{
    return QSize(640, 480);
}

void ImageCanvas::setClearColor(const QColor &color)
{
    clearColor = color;
    updateGL();
}

void ImageCanvas::updateImage(float *ptr, int w, int h, int ch)
{
    GLint format = GL_RGBA;
    switch(ch) {
        case 1: format = GL_RED; break;
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
    }
    mCurrFormat = format;
    if (texture>0)
        glDeleteTextures(1,&texture);
    // create texture object
    glGenTextures( 1, &texture );
    glBindTexture( GL_TEXTURE_2D, texture);
    // set basic parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, mCurrFormat, w, h, 0, mCurrFormat, GL_FLOAT, ptr);
    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
    update();
}

void ImageCanvas::updateTexData(const float *ptr, unsigned w, unsigned h)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mCurrFormat, GL_FLOAT, (void*)ptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ImageCanvas::initializeGL()
{
    initializeOpenGLFunctions();

    makeObject();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
#ifdef GL_TEXTURE_2D
    glEnable(GL_TEXTURE_2D);
#endif

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

    QGLShader *vshader = new QGLShader(QGLShader::Vertex, this);
    const char *vsrc =
        "attribute highp vec4 vertex;\n"
        "attribute mediump vec4 texCoord;\n"
        "varying mediump vec4 texc;\n"
        "uniform mediump mat4 matrix;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = matrix * vertex;\n"
        "    texc = texCoord;\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QGLShader *fshader = new QGLShader(QGLShader::Fragment, this);
    const char *fsrc =
        "uniform bool isGrayScale;\n"
        "uniform bool isTex;\n"
        "uniform sampler2D texture;\n"
        "varying mediump vec4 texc;\n"
        "vec4 checker(vec2 uv) {\n"
        "  float checkSize = 150.0;\n"
        "  float fmodResult = mod(floor(checkSize * uv.x) + floor(checkSize * uv.y),2.0);\n"
        "  if (fmodResult < 1.0) {\n"
        "    return vec4(1, 1, 1, 1);\n"
        "  } else {\n"
        "    return vec4(0.75, 0.75, 0.75, 1);\n"
        "  }\n"
        "}\n"
        "void main(void)\n"
        "{\n"
        "   vec4 texCol;\n"
        "    if (isTex)"
        "       texCol = texture2D(texture, vec2(texc.s,1-texc.t));\n"
        "    else"
        "       texCol = checker(texc);"
        "    if (isGrayScale)\n"
        "       gl_FragColor = vec4(texCol.r, texCol.r, texCol.r, 1.0);\n"
        "    else\n"
        "       gl_FragColor = texCol;\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    program = new QGLShaderProgram(this);
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    program->link();

    program->bind();
    program->setUniformValue("texture", 0);
}

void ImageCanvas::paintGL()
{
    qglClearColor(clearColor);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 m;
    m.ortho(-1.0f, +1.0f, +1.0f, -1.0f, 4.0f, 15.0f);
    m.translate(0.0f, 0.0f, -10.0f);

    program->setUniformValue("matrix", m);
    program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    program->setAttributeArray
        (PROGRAM_VERTEX_ATTRIBUTE, vertices.constData());
    program->setAttributeArray
        (PROGRAM_TEXCOORD_ATTRIBUTE, texCoords.constData());
    program->setUniformValue("isTex", texture>0);
    program->setUniformValue("isGrayScale", mCurrFormat==GL_RED);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void ImageCanvas::resizeGL(int width, int height)
{
    glViewport(0,0,width,height);
}

void ImageCanvas::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void ImageCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
    }
    lastPos = event->pos();
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent * /* event */)
{
    emit clicked();
}

void ImageCanvas::makeObject()
{
    static const int coords[4][3] = { { +1, -1, -1 }, { -1, -1, -1 }, { -1, +1, -1 }, { +1, +1, -1 } };
    for (int j = 0; j < 4; ++j) {
        texCoords.append
            (QVector2D(j == 0 || j == 3, j == 0 || j == 1));
        vertices.append
            (QVector3D(coords[j][0], coords[j][1],
                       coords[j][2]));
    }
}
