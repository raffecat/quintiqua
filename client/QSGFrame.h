#pragma once
#include "QSGTransformNode.h"

class QSGTexture;

class QSGFrame :
	public QSGTransformNode
{
public:
	QSGFrame(void) : m_texture(NULL),
		m_left(0), m_bottom(0), m_right(0), m_top(0)
	{
	}

	virtual ~QSGFrame(void);

public:
	virtual void renderContent(QSGRenderer* renderer);

public:
	QSGTexture* m_texture;
	float m_left;
	float m_bottom;
	float m_right;
	float m_top;
};
