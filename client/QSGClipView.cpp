#include "QSGClipView.h"
#include "QSGRenderer.h"

QSGClipView::~QSGClipView(void)
{
}

void QSGClipView::renderContent(QSGRenderer* renderer)
{
	renderer->setScissor(m_left, m_bottom, m_right, m_top);
	renderChildren(renderer);
	renderer->clearScissor();
}
