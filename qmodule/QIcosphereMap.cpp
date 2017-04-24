//
// Created by martin on 06/03/17.
//

#include "QIcosphereMap.h"
#include "../pipeline/CalenhadModel.h"
#include <QThread>
#include "../nodeedit/qneconnection.h"
#include "../nodeedit/Calenhad.h"
#include "../preferences.h"

using namespace noise::module;
using namespace noise::utils;
using namespace icosphere;

QIcosphereMap::QIcosphereMap (QWidget* parent) : QModule (nullptr), _depth (5), _bounds (Bounds()) { }

QIcosphereMap::~QIcosphereMap () { }

void QIcosphereMap::initialise() {
    QModule::initialise();
    _depthSpin = countParameterControl ("Depth");
    _depthSpin -> setMinimum (3);
    _depthSpin -> setMaximum (13);

    _vertexCountLabel = new QLabel();
    _contentLayout -> addRow (tr ("Vertices"), _vertexCountLabel);
    connect (_depthSpin, SIGNAL (editingFinished()), this, SLOT (setIcosphereDepth()));
    _contentLayout -> addRow (tr ("Depth"), _depthSpin);
    _isInitialised = true;

    connect (this, SIGNAL (nodeChanged (const QString&, const QVariant&)), this, SLOT (resetUI (const QString&)));

    setBounds (_bounds);
    setIcosphereDepth (_depth);

    emit nodeChanged();
}


void QIcosphereMap::generateMap() {
    //emit nodeChanged();
    IcosphereMap* map = new IcosphereMap();
    _module = map;
    QNEPort* port = _ports [0];
    if (! (port -> connections().isEmpty ())) {
        QNEConnection* c = port -> connections() [0];
        QNEPort* p = c -> port1 () == _ports[0] ? c -> port2() : c -> port1();
        Module* source = p -> module() -> module();
        if (source) {
            map -> SetSourceModule (0, *source);
            connect (map, SIGNAL (available (std::shared_ptr<icosphere::Icosphere>)), this, SLOT (icosphereBuilt (std::shared_ptr<icosphere::Icosphere>)));
            connect (this, SIGNAL (nodeChanged (const QString&, const QVariant&)), map, SLOT (cancelBuild()));
            map -> buildIcosphere (_depth, _bounds);
        }
    }
}

void QIcosphereMap::resetUI (const QString& type) {
    if (type == "Updated") {
        _vertexCountLabel -> setText (QString::number (_icosphere -> vertexCount ()) + " vertices generated");
    }
    if (type == "Bounds" || type == "Inputs" || type == "Depth") {
        generateMap();
    }
}

void QIcosphereMap::addInputPorts() {
    QNEPort* input = new QNEPort (QNEPort::InputPort, 0, "Input");
    addPort (input);
}

void QIcosphereMap::setUniqueName() {
    int i = 0;
    QString name = "New Icosphere";
    while (_model -> findModule (name)) {
        i++;
        name = QString ("New Icosphere " + QString::number (i));
    }
    setName (name);
    _nameEdit -> setText (_name);
}

void QIcosphereMap::setIcosphereDepth() {
    int depth = _depthSpin -> value();
    setIcosphereDepth (depth);
}

void QIcosphereMap::setIcosphereDepth (const unsigned& depth) {
    if (_depth > 2 && depth < 14) {
        _depth = depth;
        if (_depthSpin -> value () != depth) {
            _depthSpin -> setValue (depth);
        }
        emit nodeChanged();
    }
}

void QIcosphereMap::icosphereBuilt (std::shared_ptr<icosphere::Icosphere> icosphere) {
    if (icosphere != _icosphere) {
        _icosphere = icosphere;
    }
    _vertexCountLabel -> setText (QString::number (_icosphere -> vertexCount ()) + " vertices generated");
    emit nodeChanged();
}

void QIcosphereMap::setBounds (const icosphere::Bounds& bounds) {
    _bounds = bounds;
    generateMap();
    emit nodeChanged();
}


bool QIcosphereMap::isRenderable() {
    return _module != nullptr && QNode::isRenderable();
}

IcosphereMap* QIcosphereMap::module () {
    IcosphereMap* map = dynamic_cast<IcosphereMap*> (_module);
    return map;
}

QIcosphereMap* QIcosphereMap::newInstance() {
    QIcosphereMap* qm = new QIcosphereMap();
    qm -> initialise();
    return qm;
}

QString QIcosphereMap::moduleType () {
    return CalenhadServices::preferences() -> calenhad_module_icospheremap;
}

QIcosphereMap* QIcosphereMap::addCopy (CalenhadModel* model)  {
    QIcosphereMap* qm = QIcosphereMap::newInstance();
    if (qm) {
        qm -> setModel (model);
        qm -> setIcosphereDepth (_depth);
    }
    return qm;
}

void QIcosphereMap::inflate (const QDomElement& element) {
    QModule::inflate (element);
    bool ok;
    double depth = _model -> readParameter (element, "depth").toDouble (&ok);
    if (ok) { setIcosphereDepth (depth); }

    QDomElement boundsElement = element.firstChildElement ("bounds");
    QDomElement latlonElement1 = boundsElement.firstChildElement ("geolocation");
    double lat1 = M_PI, lat2 = -M_PI, lon1 = -M_PI * 2, lon2 = M_PI * 2;
    lat1 = latlonElement1.attributeNode ("lat").value ().toDouble (&ok);
    if (ok) { lon1 = latlonElement1.attributeNode ("lon").value ().toDouble (&ok); }

    QDomElement latlonElement2 = latlonElement1.nextSiblingElement ("geolocation");
    if (ok) { lat2 = latlonElement2.attributeNode ("lat").value ().toDouble (&ok); }
    if (ok) { lon2 = latlonElement2.attributeNode ("lon").value ().toDouble (&ok); }
    if (ok) {
        bool crossesDateline = boundsElement.attributeNode ("crossesDateline").value ().toLower () == "y";
        Bounds bounds = Bounds (lat1, lat2, lon1, lon2, crossesDateline);
        setBounds (bounds);
    }
}

void QIcosphereMap::serialise (QDomDocument& doc) {
    QModule::serialise (doc);
    _model -> writeParameter (_element, "depth", QString::number (_depth));
    QDomElement boundsElement = doc.createElement ("bounds");
    _element.appendChild (boundsElement);
    QDomElement latlonElement = doc.createElement ("geolocation");
    boundsElement.appendChild (latlonElement);
    latlonElement.setAttribute ("lat", _bounds.lat1);
    latlonElement.setAttribute ("lon", _bounds.lon1);
    latlonElement = doc.createElement ("geolocation");
    boundsElement.appendChild (latlonElement);
    latlonElement.setAttribute ("lat", _bounds.lat2);
    latlonElement.setAttribute ("lon", _bounds.lon2);
    boundsElement.setAttribute ("crossesDateline", _bounds._crossesDateline ? "y" : "n");
}

int QIcosphereMap::icosphereDepth () {
    return _depth;
}
