#include "CalenhadMapWidget.h"
#include <QIcon>

#include "../graph/graph.h"
#include "../CalenhadServices.h"
#include "../preferences/preferences.h"
#include "projection/ProjectionService.h"
#include "projection/Projection.h"
#include <QWindow>
#include <QtXml/QtXml>
#include <QtGui/QPainter>
#include <controls/globe/CalenhadGlobeConstants.h>
#include <GeographicLib/Geodesic.hpp>
#include <QtWidgets/QToolTip>
#include "Graticule.h"
#include "../qmodule/Module.h"


using namespace calenhad;
using namespace geoutils;
using namespace matrices;
using namespace icosphere;
using namespace calenhad::graph;
using namespace calenhad::mapping;
using namespace calenhad::mapping::projection;
using namespace calenhad::pipeline;
using namespace calenhad::qmodule;
using namespace calenhad::controls::globe;
using namespace calenhad::legend;
using namespace GeographicLib;

CalenhadMapWidget::CalenhadMapWidget (QWidget* parent) : QOpenGLWidget (parent),
     _datumFormat (DatumFormat::Scaled),
    _zoomDrag (false),
    _graph (nullptr),
    _sensitivity (0.5),
    _mouseDoubleClickMode (CalenhadGlobeDoubleClickMode::Goto),
    _bounds (Bounds (-M_PI_2, M_PI_2, 0, M_2_PI)),
    _source (nullptr), _previewType (OverviewPreviewType::WholeWorld),
    _geodesic (new Geodesic (1, 0)),
    _vertexBuffer (nullptr),
    _computeShader (nullptr),
    _computeProgram (nullptr),
    _vertexShader (nullptr),
    _fragmentShader (nullptr),
    _globeTexture (nullptr),
    _renderProgram (nullptr),
    _indexBuffer (nullptr),
    _colorMapBuffer (nullptr),
    _heightMapBuffer (nullptr),
    _projection (CalenhadServices::projections() -> fetch ("Equirectangular")),
    _scale (1.0),
    _shader (""),
    _graticule (nullptr),
    _graticuleVisible (true),
    _coordinatesFormat (CoordinatesFormat::Traditional),
    _inset (false),
    _rotation (Geolocation (0, 0)),
    _insetHeight (CalenhadServices::preferences() -> calenhad_globe_inset_height) {

    QSurfaceFormat format;
    format.setSamples(8);
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
    setContextMenuPolicy(Qt::CustomContextMenu);

    // read shader code from files into memory for use at render time
    QFile csFile (":/shaders/map_cs.glsl");
    csFile.open (QIODevice::ReadOnly);
    QTextStream csTextStream (&csFile);
    _shaderTemplate = csTextStream.readAll ();

    QFile vsFile (":/shaders/map_vs.glsl");
    vsFile.open (QIODevice::ReadOnly);
    QTextStream vsTextStream (&vsFile);
    _vertexShaderCode = vsTextStream.readAll ();

    QFile fsFile (":/shaders/map_fs.glsl");
    fsFile.open (QIODevice::ReadOnly);
    QTextStream fsTextStream (&fsFile);
    _fragmentShaderCode = fsTextStream.readAll ();

    _graticule = new Graticule (this);

}

CalenhadMapWidget::~CalenhadMapWidget() {
    makeCurrent();
    if (_computeShader) { delete _computeShader; }
    if (_vertexShader) { delete _vertexShader; }
    if (_fragmentShader) { delete _fragmentShader; }
    if (_globeTexture) { delete _globeTexture; }
    if (_heightMapBuffer) { delete _heightMapBuffer; }
    if (_renderProgram) { delete _renderProgram; }
    if (_computeProgram) { delete _computeProgram; }
    if (_indexBuffer)  { delete _indexBuffer; }
    if (_vertexBuffer) { delete _vertexBuffer; }
    if (_graticule) { delete _graticule; }
    delete _geodesic;
}

void CalenhadMapWidget::initializeGL() {
    if (_graph) {
        initializeOpenGLFunctions ();
        glEnable (GL_MULTISAMPLE);

        glClearColor (0, 0, 1, 1);
        _colorMapBuffer = _graph->colorMapBuffer ();


        m_vao.create ();
        if (m_vao.isCreated ()) {
            m_vao.bind ();
        }

        static const GLfloat g_vertex_buffer_data[] = {
                -1.0f, -1.0f,
                -1.0f, 1.0f,
                1.0f, -1.0f,
                1.0f, 1.0f
        };
        static const GLushort g_element_buffer_data[] = {0, 1, 2, 3};

        _vertexBuffer = new QOpenGLBuffer (QOpenGLBuffer::VertexBuffer);
        _vertexBuffer->create ();
        _vertexBuffer->setUsagePattern (QOpenGLBuffer::StaticDraw);
        _vertexBuffer->bind ();
        _vertexBuffer->allocate (g_vertex_buffer_data, sizeof (g_vertex_buffer_data));

        _indexBuffer = new QOpenGLBuffer (QOpenGLBuffer::IndexBuffer);
        _indexBuffer->create ();
        _indexBuffer->setUsagePattern (QOpenGLBuffer::StaticDraw);
        _indexBuffer->bind ();
        _indexBuffer->allocate (g_element_buffer_data, sizeof (g_element_buffer_data));

        // texture for the map
        glActiveTexture (GL_TEXTURE0);
        _globeTexture = new QOpenGLTexture (QOpenGLTexture::Target2D);
        _globeTexture->create ();
        _globeTexture->setFormat (QOpenGLTexture::RGBA8_UNorm);
        _globeTexture->setSize (height() * 2, height());
        _globeTexture->setMinificationFilter (QOpenGLTexture::Linear);
        _globeTexture->setMagnificationFilter (QOpenGLTexture::Linear);
        _globeTexture->allocateStorage ();
        _globeTexture->bind ();
        _heightMapBuffer = new float[_globeTexture->height () * _globeTexture->width ()];
        glBindImageTexture (0, _globeTexture->textureId (), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

        _computeShader = new QOpenGLShader (QOpenGLShader::Compute);
        _vertexShader = new QOpenGLShader (QOpenGLShader::Vertex);
        _vertexShader->compileSourceCode (_vertexShaderCode);
        _fragmentShader = new QOpenGLShader (QOpenGLShader::Fragment);
        _fragmentShader->compileSourceCode (_fragmentShaderCode);
        _computeProgram = new QOpenGLShaderProgram ();
        clock_t start = clock ();
        _computeShader->compileSourceCode (_shader);
        _computeProgram->removeAllShaders ();
        _computeProgram->addShader (_computeShader);
        _computeProgram->link ();
        _computeProgram->bind ();

        clock_t end = clock ();
        std::cout << "Compile shader " << ((double) end - (double) start) / CLOCKS_PER_SEC * 1000.0 << " milliseconds\n";
        _renderProgram = new QOpenGLShaderProgram ();
        _renderProgram->addShader (_vertexShader);
        _renderProgram->addShader (_fragmentShader);
        _renderProgram->link ();
        _renderProgram->bind ();

        GLint posPtr = glGetAttribLocation (_renderProgram->programId (), "pos");
        glVertexAttribPointer (posPtr, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray (posPtr);

        m_vao.release ();
    }
}

void CalenhadMapWidget::compute () {
    if (_graph) {
        clock_t start = clock ();
        makeCurrent ();

        static GLint destLoc = glGetUniformLocation (_computeProgram->programId (), "destTex");
        static GLint insetLoc = glGetUniformLocation (_computeProgram->programId (), "insetTex");
        static GLint cmbsLoc = glGetUniformLocation (_computeProgram->programId (), "colorMapBufferSize");
        static GLint resolutionLoc = glGetUniformLocation (_computeProgram->programId (), "resolution");
        static GLint projectionLoc = glGetUniformLocation (_computeProgram->programId (), "projection");
        static GLint scaleLoc = glGetUniformLocation (_computeProgram->programId (), "scale");
        static GLint datumLoc = glGetUniformLocation (_computeProgram->programId (), "datum");
        static GLint insetHeightLoc = glGetUniformLocation (_computeProgram->programId (), "insetHeight");
        static GLint rasterResolutionLoc = glGetUniformLocation (_computeProgram->programId (), "rasterResolution");

        m_vao.bind ();
        _computeProgram->bind ();
        _globeTexture->bind ();

        GLubyte c = 0;
        std::vector<GLubyte> emptyData (_globeTexture->width () * _globeTexture->height () * 4, 0);
        glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, _globeTexture->width (), _globeTexture->height (), GL_BGRA, GL_UNSIGNED_BYTE, &emptyData[0]);
        glUniform1i (destLoc, 0);
        glUniform1i (insetLoc, 1);
        glUniform2f (datumLoc, (GLfloat) _rotation.longitude(), (GLfloat) _rotation.latitude());
        glUniform1i (projectionLoc, _projection->id ());
        glUniform1f (scaleLoc, (GLfloat) _scale);
        glUniform1i (insetHeightLoc, _inset ? _insetHeight : 0);
        glUniform1i (resolutionLoc, _globeTexture -> height());
        glUniform1i (cmbsLoc, 2048);
        glUniform1i (rasterResolutionLoc, _globeTexture -> height());

        // create and allocate the colorMapBuffer on the GPU and copy the contents across to them.
        _colorMapBuffer = _graph->colorMapBuffer ();
        if (_colorMapBuffer) {
            GLuint colorMap = 1;
            glGenBuffers (1, &colorMap);
            glBindBuffer (GL_SHADER_STORAGE_BUFFER, colorMap);
            glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * _graph->colorMapBufferSize (), _colorMapBuffer, GL_DYNAMIC_COPY);
            glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, colorMap);
            glBindBuffer (GL_SHADER_STORAGE_BUFFER, 1); // unbind
        }

        // create and allocate a buffer for any input rasters
        int rasters = _graph->rasterCount ();
        glActiveTexture (GL_TEXTURE1);
        _rasterTexture = new QOpenGLTexture (QOpenGLTexture::Target2DArray);
        _rasterTexture->create ();
        _rasterTexture->setFormat (QOpenGLTexture::RGBA8_UNorm);
        _rasterTexture->setSize (_globeTexture -> width(), _globeTexture -> height());
        _rasterTexture->setMinificationFilter (QOpenGLTexture::Linear);
        _rasterTexture->setMagnificationFilter (QOpenGLTexture::Linear);
        _rasterTexture->allocateStorage ();
        _rasterTexture->bind ();
        glBindImageTexture (1, _rasterTexture->textureId (), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
        for (int i = 0; i < rasters; i++) {
            QImage* raster = _graph->raster (i);
            _rasterTexture->setData (0, i, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, (void*) raster->bits ());
        }

        // create and allocate the heightMapBuffer on the GPU. This is for downloading the heightmap from the GPU.
        GLuint heightMap = 1;

        glGenBuffers (1, &heightMap);
        glBindBuffer (GL_SHADER_STORAGE_BUFFER, heightMap);
        glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (GLfloat) * _globeTexture -> height() * _globeTexture -> width(), NULL, GL_DYNAMIC_READ);
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 3, heightMap);
        glBindBuffer (GL_SHADER_STORAGE_BUFFER, 1); // unbind
        int x = _globeTexture -> width() / 16;
        int y = _globeTexture -> height() / 16;
        std::cout << "glDispatchCompute (" << x << ", " << y << ", 1)\n";
        glDispatchCompute (x, y, 1);
        glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);

        // retrieve the height map data from the GPU
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 3, heightMap);
        glBindBuffer (GL_SHADER_STORAGE_BUFFER, heightMap);
        GLfloat* heightData = (GLfloat*) glMapBuffer (GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
        memcpy (_heightMapBuffer, heightData, _globeTexture -> height () * _globeTexture -> width () * sizeof (GLfloat));

        clock_t end = clock ();
        std::cout << "Computed in " << ((double) end - (double) start) / CLOCKS_PER_SEC * 1000.0 << " milliseconds\n";
    }
}

void CalenhadMapWidget::paintGL() {
    if (_graph) {
        QPainter p (this);
        p.beginNativePainting ();
        compute ();
        // draw
        _renderProgram->bind ();
        _globeTexture->bind ();
        static GLint srcLoc = glGetUniformLocation (_renderProgram->programId (), "srcTex");
        glUniform1i (srcLoc, 0);
        glDrawElements (GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
        m_vao.release ();

        glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);

        p.endNativePainting ();

        // draw the graticule
        if (_graticule && _graticuleVisible) {
            _graticule->drawGraticule (p);
        }

        emit rendered (true);
    }
}

GLfloat* CalenhadMapWidget::heightMapBuffer() const {
    return _heightMapBuffer;
}

QSize CalenhadMapWidget::heightMapSize() const {
    return QSize (_globeTexture -> width(), _globeTexture -> height());
}


void CalenhadMapWidget::resizeGL (int width, int height) {
    if (_graph) {
        if (height == 0) height = 1;
        float aspectRatio = (float) width / (float) height;
        glViewport (0, 0, width, height);

        glMatrixMode (GL_PROJECTION); /* switch to projection matrix */
        glLoadIdentity ();
        if (width >= height) {
            glOrtho (-0.5f * aspectRatio, 0.5f * aspectRatio, 0.0f, 1.0f, -1, 1);
        } else {
            glOrtho (-0.5f, 0.5f, 0.0, 1.0 / aspectRatio, -1, 1);
        }
        glMatrixMode (GL_MODELVIEW);
    }
}

// Insert the given code into the compute shader to realise the noise pipeline
void CalenhadMapWidget::setGraph (Graph* g) {
    if (_graph != g) {
        _graph = g;
        makeCurrent ();
        _shader = _shaderTemplate;
        QString code = g->glsl ();
        if (code != QString::null) {
           // std::cout << code.toStdString () << "\n";
            _code = code;
            _shader.replace ("// inserted code //", _code);
            _shader.replace ("// inserted inverse //", CalenhadServices::projections() -> glslInverse ());
            _shader.replace ("// inserted forward //", CalenhadServices::projections() -> glslForward ());

            if (_computeShader) {
                _computeProgram -> removeAllShaders ();
                if (_computeShader -> compileSourceCode (_shader)) {
                    _computeProgram -> addShader (_computeShader);
                    _computeProgram -> link ();
                    _computeProgram -> bind ();
                } else {
                    std::cout << "Compute shader would not compile\n";
                    emit rendered (false);
                }
            } else {
                std::cout << "No compute shader\n";
                emit rendered (false);
            }
        } else {
            emit rendered (false);
        }
    }
}

void CalenhadMapWidget::showEvent (QShowEvent* e) {
    //updateGL();
}


void CalenhadMapWidget::setScale (const double& scale) {
    _scale = scale;
}

double CalenhadMapWidget::scale () {
    return _scale;
}

void CalenhadMapWidget::setProjection (const QString& projection) {
    _projection = CalenhadServices::projections () -> fetch (projection);
    update();
}

Projection* CalenhadMapWidget::projection() {
    return _projection;
}

void CalenhadMapWidget::rotate (const Geolocation& rotation) {
    _rotation = rotation;
    update();
}

const Geolocation& CalenhadMapWidget::rotation () {
    return _rotation;
}

Bounds CalenhadMapWidget::bounds() {

}

void CalenhadMapWidget::setInset (bool inset) {
    _inset = inset;
    update();
}

bool CalenhadMapWidget::isInViewport (Geolocation g) {
    QPointF s;
    if (screenCoordinates (g, s)) {
        return s.x() >= 0 && s.x() <= width() && s.y() >= 0 && s.y() < height();
    } else {
        return false;
    }
}

QImage* CalenhadMapWidget::heightmap() {
    QImage* image = new QImage (_globeTexture -> width(), _globeTexture -> height(), QImage::Format_ARGB32);
    image -> fill (Qt::red);
    int w = _globeTexture -> width();
    int h = _globeTexture -> height();
    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            float value = (float) _heightMapBuffer [y * w + x];
            value = std::min (std::max (-1.0f, value), 1.0f);
            int k = (int) ((value + 1) / 2 * 256);
            image -> setPixelColor (x, y, QColor (k, k, k));
        }
    }
    return image;
}

bool CalenhadMapWidget::valueAt (const QPointF& sc, double& value) {
    QPoint tc = texCoordinates (sc);
    int index = tc.y() * _globeTexture -> width () + tc.x();
    if (index >= 0 && index < _globeTexture -> width () * _globeTexture -> width()) {
        value = (double) _heightMapBuffer [index];
        return true;
    } else {
        return false;
    }
}

QPoint CalenhadMapWidget::texCoordinates (const QPointF& sc) {
    if (_globeTexture) {
        QPoint tc;
        double x = sc.x () / width ();
        double y = sc.y () / height ();
        tc.setX (x * _globeTexture -> width ());
        tc.setY ((1 - y) * _globeTexture -> height ());
        return tc;
    } else {
        return QPoint();
    }
}

bool CalenhadMapWidget::screenCoordinates (Geolocation geolocation, QPointF& sc) {
    _projection -> setDatum (_rotation);
    QPointF p;
    bool fwd = _projection -> forward (geolocation, p);
    if (fwd) {
        double x = p.x () / _scale;
        double y = (-p.y ()) / _scale;
        x += M_PI;
        y += M_PI / 2;
        x /= M_PI * 2;
        y /= M_PI;
        sc.setX (x * width ());
        sc.setY (y * height ());
    }
    return fwd;
}

bool CalenhadMapWidget::geoCoordinates (QPointF pos, Geolocation& geolocation) {
    double x = ((pos.x() / width()) * M_PI * 2 - M_PI) * _scale;
    double y = ((pos.y() / height()) * M_PI - (M_PI / 2)) * _scale;
    _projection -> setDatum (_rotation);
    bool result = _projection -> inverse (QPointF (x, -y), geolocation);
    return result;
}

bool CalenhadMapWidget::inset () {
    return _inset;
}


QRectF CalenhadMapWidget::insetRect() {
    double h = (_insetHeight / (double) _globeTexture -> height()) * height();
    double y = (1 - ( _insetHeight / (double) _globeTexture -> height())) * height();
    return QRectF (0, y, h * 2, h);
}


void CalenhadMapWidget::setGraticuleVisible (const bool& visible) {
    _graticuleVisible = visible;
    update();
}

bool CalenhadMapWidget::isGraticuleVisible() {
    return _graticuleVisible;
}

Graticule* CalenhadMapWidget::graticule() {
    return _graticule;
}

void CalenhadMapWidget::setCoordinatesFormat (CoordinatesFormat format) {
    _coordinatesFormat = format;
}

geoutils::CoordinatesFormat CalenhadMapWidget::coordinatesFormat () {
    return _coordinatesFormat;
}

Statistics CalenhadMapWidget::statistics() {
    double _min = 0, _max = 0, _sum = 0;
    int count = 0;
    for (int i = 0; i < _globeTexture -> height() * _globeTexture -> width(); i++) {
        if (!isnan (_heightMapBuffer[i])) {
            if (_heightMapBuffer[i] < _min) { _min = _heightMapBuffer[i]; }
            if (_heightMapBuffer[i] > _max) { _max = _heightMapBuffer[i]; }
            _sum += _heightMapBuffer[i];
            count++;
        }
    }

    Statistics statistics = Statistics (_min, _max, _sum, count);
    return statistics;
}

void CalenhadMapWidget::paintEvent (QPaintEvent* e) {
    QOpenGLWidget::paintEvent (e);
}

void CalenhadMapWidget::render() {

    if (_source -> isComplete()) {
        Graph* g = new Graph (_source);
        setGraph (g);
    }

}

Module* CalenhadMapWidget::source() {
    return _source;
}

void CalenhadMapWidget::setSource (Module* qm) {
    _source = qm;
    connect (qm, &Node::nodeChanged, this, &CalenhadMapWidget::render);
    render();
}

// Zoom so that the region enclosed in the whole viewport is compressed into the target _box.
void CalenhadMapWidget::zoomOutFrom (const Bounds& target) {
    /*double newRadius = _bounds.width ();
    Geolocation centre = target.center ();

    if(target.height() && target.width()) {
        //work out the needed scale level
        QPointF nw, se;
        double nx, sx, wx, ex;
        screenCoordinates (Geolocation (target.north(), target.west()), nw);
        screenCoordinates (Geolocation (target.east(), target.south()), se);
        double const horizontalRadius = (std::abs (nw.x() - se.x()) / _bounds.height()) / ( 0.25 * M_PI );
        double const verticalRadius = (std::abs (ex - wx) / _bounds.width()) / ( 0.25 * M_PI );
        std::cout << horizontalRadius << " " << verticalRadius << "\n";
        newRadius = qMin<double>(horizontalRadius, verticalRadius );
        newRadius = qMax<double> (100, qMin<double>(newRadius, 1e+08));
    }
    goTo (target.center()); // in radians
    _zoomSlider -> setValue (radiusToZoomFactor (newRadius));
    */
}

// Zoom so that the region enclosed in the target _box fills the whole viewport.
void CalenhadMapWidget::zoomInTo (const Bounds& target) {
    /*double newRadius = std::max (_bounds.width (), _bounds.height ());
    if(target.height() && target.width()) {
        double const horizontalRadius = ( 0.25 * M_PI ) * (_bounds.height() / target.height());
        double const verticalRadius = ( 0.25 * M_PI ) * (_bounds.width() / target.width());
        newRadius = qMin<double>(horizontalRadius, verticalRadius );
        newRadius = qMax<double> (100, qMin<double>(newRadius, 1e+08));
    }
    goTo (target.center()); // in radians
    _zoomSlider -> setValue (radiusToZoomFactor (newRadius)); */
}

void CalenhadMapWidget::setDatumFormat (DatumFormat format) {
    _datumFormat = format;
}

void CalenhadMapWidget::setSensitivity (double sensitivity) {
    _sensitivity = sensitivity;
}

DatumFormat CalenhadMapWidget::datumFormat () {
    return _datumFormat;
}

double CalenhadMapWidget::sensitivity () {
    return _sensitivity;
}

void CalenhadMapWidget::goTo (const Geolocation& geolocation) {
    rotate (geolocation);
}

void CalenhadMapWidget::mousePressEvent (QMouseEvent* e) {
    Geolocation loc;
    if (e -> button () == Qt::LeftButton) {
        if (geoCoordinates (e -> pos(), loc)) {
            _moveFrom = e -> pos ();
            setCursor (Qt::OpenHandCursor);
        }
    }
}

void CalenhadMapWidget::mouseDoubleClickEvent (QMouseEvent* e) {
    if (_mouseDoubleClickMode == CalenhadGlobeDoubleClickMode::Goto) {
        Geolocation loc;
        if (geoCoordinates (e -> pos(), loc)) {
            goTo (loc);
        }
    }
    if (_mouseDoubleClickMode == CalenhadGlobeDoubleClickMode::Place) {
        // to do
    }
}

void CalenhadMapWidget::mouseMoveEvent (QMouseEvent* e) {
    Geolocation se, nw;

    if (e -> buttons() & Qt::LeftButton) {
        if (cursor().shape () != Qt::ClosedHandCursor) {
            setCursor (Qt::ClosedHandCursor);
        }

        double dx = e -> pos().x() - _moveFrom.x();
        double dy = e -> pos().y() - _moveFrom.y();

        if (_mouseDragMode == CalenhadGlobeDragMode::Pan) {
            double dLon = 180.0 * _scale * _sensitivity * dx / 50;
            double dLat = 180.0 * _scale * _sensitivity * dy / 50;
            _moveFrom = e -> pos();
            goTo (Geolocation (_rotation.latitude() + qDegreesToRadians (dLat), _rotation.longitude() - qDegreesToRadians (dLon)));
        }

        if (_mouseDragMode == CalenhadGlobeDragMode::Zoom) {
            double dz = dy * _sensitivity / 10;
            emit zoomRequested (_scale + dz);
            _moveFrom = e -> pos();
            update();
        }
    } else {
        setCursor (Qt::CrossCursor);
        if  ( _coordinatesFormat != CoordinatesFormat::NoCoordinates) {
            Geolocation loc;
            QPointF point;
            point.setX ((double) e -> pos().x());
            point.setY ((double) e -> pos().y());

            if (geoCoordinates (point, loc)) {
                QString text = geoutils::Math::geoLocationString (loc, _coordinatesFormat);
                double value;
                if (_globeTexture) {
                    QPoint tc = texCoordinates (point);
                    if (isInViewport (loc)) {
                        text += ": " + QString::number (tc.x()) + ", " + QString::number (tc.y());
                        text += ": " + QString::number (tc.y() * _globeTexture -> width() + tc.x()) + " ";
                        if (valueAt (point, value)) {
                            text += ": " + QString::number (value);
                        }
                        QToolTip::showText (e->globalPos (), text, this);
                    }
                }
            }
        }
    }
}

void CalenhadMapWidget::wheelEvent (QWheelEvent* event) {
    double dz =  - event -> delta() * _sensitivity / 1200;
    emit zoomRequested (_scale + dz);
    update();
}

void CalenhadMapWidget::mouseReleaseEvent (QMouseEvent* e) {
    _zoomDrag = false;
    if (_mouseDragMode == CalenhadGlobeDragMode::Zoom) {
        if (e -> modifiers () & Qt::ControlModifier) {
            zoomOutFrom (_zoomBox);
        } else {
            zoomInTo (_zoomBox);
        }
    }
    Geolocation loc;

    // rollback cursor for mouse move without buttons
    if (geoCoordinates (e -> pos(), loc)) {
        setCursor (Qt::CrossCursor);
    } else {
        setCursor (Qt::ArrowCursor);
    }
}


void CalenhadMapWidget::setMouseDoubleClickMode (const CalenhadGlobeDoubleClickMode& mode) {
    _mouseDoubleClickMode = mode;
}

void CalenhadMapWidget::setMouseDragMode (const CalenhadGlobeDragMode& mode) {
    _mouseDragMode = mode;
}

CalenhadGlobeDoubleClickMode CalenhadMapWidget::mouseDoubleClickMode () {
    return _mouseDoubleClickMode;
}

CalenhadGlobeDragMode CalenhadMapWidget::mouseDragMode () {
    return _mouseDragMode;
}

// move the view centre along a given azimuth
void CalenhadMapWidget::navigate (const NavigationEvent& e) {
    // move the viewport centre in the chosen direction by the distance multiplied by the current scale
    double lat, lon;
    double distance = e.distance() * scale();
    _geodesic -> Direct (rotation().latitude (Units::Degrees), rotation().longitude (Units::Degrees), e.azimuth(), distance, lat, lon);
    goTo (Geolocation (lat, lon, Units::Degrees));
}