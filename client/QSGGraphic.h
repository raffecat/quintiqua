#pragma once
#include "QSGTransformNode.h"
#include "QSGTexture.h"
#include "QSGGeometry.h"
#include <vector>


class QSGGraphic :
	public QSGTransformNode
{
public:
	QSGGraphic(void) {}
	virtual ~QSGGraphic(void);

public:
	virtual void renderContent(QSGRenderer* renderer);

public:
	ref_ptr<QSGTexture> m_texture;
	QSGGeometry m_geometry;
};
