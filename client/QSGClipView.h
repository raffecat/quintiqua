#pragma once
#include "QSGTransformNode.h"

class QSGClipView :
	public QSGTransformNode
{
public:
	QSGClipView(void) {}
	virtual ~QSGClipView(void);

public:
	virtual void renderContent(QSGRenderer* renderer);

public:
	float m_left;
	float m_bottom;
	float m_right;
	float m_top;
};
