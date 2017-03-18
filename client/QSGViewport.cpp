#include "QSGViewport.h"
#include "QSGRenderer.h"

QSGViewport::~QSGViewport(void)
{
}

void QSGViewport::render(QSGRenderer* renderer)
{
	renderer->clear(m_backgroundColour);
	renderChildren(renderer);
}
