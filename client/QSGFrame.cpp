#include "QSGFrame.h"
#include "QSGRenderer.h"

QSGFrame::~QSGFrame(void)
{
}

void QSGFrame::renderContent(QSGRenderer* renderer)
{
	if (m_texture) renderer->setTexture(m_texture);
	else renderer->clearTexture();

	renderer->renderQuad(m_left, m_bottom, m_right, m_top);

	renderChildren(renderer);
}
