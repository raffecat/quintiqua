#pragma once
#include <memory>
#include "QSGObject.h"

class QSGRenderData;

class QSGResource :
	public QSGObject
{
public:
	QSGResource(void) : m_renderData(0) {}
	virtual ~QSGResource(void);


public: // for QSGRenderer
	unsigned long m_renderData;
};
