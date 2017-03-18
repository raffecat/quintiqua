#pragma once
#include "QSGObject.h"
#include <list>

class QSGNode :
	public QSGObject
{
public:
	QSGNode(void) : m_parent(NULL) {}
	virtual ~QSGNode(void);

public:
	// Render this node into the QSGRenderer visitor.
	// NB. the visitor is valid for the duration of this call only.
	// By default this calls renderChildren; override in subclasses
	// to render something more interesting.
	virtual void render(class QSGRenderer* renderer) = 0;

	// Render all child nodes owned by this QSGNode.
	// This is intended to be called by subclasses.
	virtual void renderChildren(QSGRenderer* renderer);

	// No actual requirements in mind, so these are fairly arbitrary.
	virtual void appendChild(QSGNode* child);
	virtual void insertChild(size_t index, QSGNode* child);
	virtual void removeChild(QSGNode* child);
	virtual void removeAllChildren(void);

	inline QSGNode* getParent(void) { return m_parent; }

protected:
	QSGNode* m_parent;
	typedef std::list<QSGNode*> nodeListType;
	nodeListType m_children;
};
