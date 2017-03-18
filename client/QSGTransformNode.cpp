#include "QSGTransformNode.h"
#include "QSGRenderer.h"

QSGTransformNode::~QSGTransformNode(void)
{
}


void QSGTransformNode::render(QSGRenderer* renderer)
{
	renderer->pushTransform(&m_transform);
	this->renderContent(renderer);
	renderer->popTransform();
}

void QSGTransformNode::renderContent(QSGRenderer* renderer)
{
	this->renderChildren(renderer);
}
