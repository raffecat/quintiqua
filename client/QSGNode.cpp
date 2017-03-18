#include "QSGNode.h"

QSGNode::~QSGNode(void)
{
}

void QSGNode::renderChildren(QSGRenderer* renderer)
{
	for (nodeListType::iterator it = m_children.begin();
		it != m_children.end(); ++it)
	{
		(*it)->render(renderer);
	}
}

void QSGNode::appendChild(QSGNode* child)
{
	if (child->m_parent) child->m_parent->removeChild(child);
	m_children.push_back(child);
	child->m_parent = this;
}

void QSGNode::insertChild(size_t index, QSGNode* child)
{
	if (child->m_parent) child->m_parent->removeChild(child);
	if (index >= m_children.size()) m_children.push_back(child);
	else {
		nodeListType::iterator before = m_children.begin();
		while (index--) ++before; // not so good.
		m_children.insert(before, child);
	}
	child->m_parent = this;
}

void QSGNode::removeChild(QSGNode* child)
{
	m_children.remove(child);
}

void QSGNode::removeAllChildren(void)
{
	for (nodeListType::iterator it = m_children.begin();
		 it != m_children.end(); ++it)
	{
		(*it)->m_parent = NULL;
	}
	m_children.clear();
}
