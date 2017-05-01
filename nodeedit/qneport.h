/* Copyright (c) 2012, STANISLAW ADASZEWSKI
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of STANISLAW ADASZEWSKI nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL STANISLAW ADASZEWSKI BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#ifndef QNEPORT_H
#define QNEPORT_H

#include <QGraphicsPathItem>
#include <libnoise/module/modulebase.h>

class QNEBlock;
class QNEConnection;
class EditableLabel;
class QNode;

enum PortHighlight { NONE, CAN_CONNECT, CONNECTED };

class QNEPort : public QObject, public QGraphicsPathItem {
    Q_OBJECT
public:
	enum { Type = QGraphicsItem::UserType + 1 };
	enum { ControlPort = 0, InputPort = 1, OutputPort = 2 };

    QNEPort (int type, int index, const QString& name, QNEBlock *parent = 0);
	~QNEPort();

	void setBlock (QNEBlock*);

	int index();
	int radius();
	QVector<QNEConnection*>& connections();
	void initialise ();
	QString& portName();
	int portType() const { return _portType; }
	bool hasConnection();
	int type() const { return Type; }
	QNEBlock* block() const;
	void addConnection (QNEConnection* c);
	void removeConnection(QNEConnection* c);
    virtual QRectF boundingRect() const;
	bool isConnected (QNEPort*);
	QNode* owner ();
	void invalidateRenders ();
	void setHighlight (const PortHighlight& highlight);
    Q_PROPERTY (QString name READ portName WRITE setName MEMBER _portName);

public slots:
    void setName (const QString &n);
	void nameChangeRequested (const QString& value);

    signals:
    void connected (QNEConnection* c);
    void disconnected (QNEPort* port);

protected:
	QVariant itemChange (GraphicsItemChange change, const QVariant &value);

private:
	QNEBlock *_block;
	QString _portName;
	bool isOutput_;
	EditableLabel*_label = nullptr;
	int _radius;
	int _margin;
	QVector<QNEConnection*> m_connections;
	int _portType;
	int _index;
	quint64 _ptr;


};

#endif // QNEPORT_H
