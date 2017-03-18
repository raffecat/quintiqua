#include "QSGGraphic.h"
#include "QSGRenderer.h"

QSGGraphic::~QSGGraphic(void)
{
}

void QSGGraphic::renderContent(QSGRenderer* renderer)
{
	if (m_texture) renderer->setTexture(m_texture);
	else renderer->clearTexture();

	renderer->renderGeometry(&m_geometry);

	// renderChildren(renderer); // not meant to have children.
}
