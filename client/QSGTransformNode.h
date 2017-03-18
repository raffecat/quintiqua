#pragma once
#include "QSGNode.h"
#include "QSGTransform.h"

class QSGTransformNode :
	public QSGNode
{
public:
	QSGTransformNode(void) {}
	virtual ~QSGTransformNode(void);

public:
	virtual void render(QSGRenderer* renderer);
	virtual void renderContent(QSGRenderer* renderer);

public:
	QSGTransform m_transform;
};
