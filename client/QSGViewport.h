#pragma once
#include "QSGNode.h"
#include "QSGTransform.h"

class QSGViewport :
	public QSGNode
{
public:
	QSGViewport() : m_backgroundColour(QSGBlack) {}
	virtual ~QSGViewport(void);

public:
	// Change the background colour of the scene.
	void setBackground(QSGColour colour)
	{
		m_backgroundColour = colour;
	}

public:
	virtual void render(class QSGRenderer* renderer);

protected:
	QSGColour m_backgroundColour;
	ref_ptr<QSGRenderer> m_renderer;
};
