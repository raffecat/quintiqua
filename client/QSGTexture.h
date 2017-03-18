#pragma once
#include "QSGResource.h"
#include <string>

class QSGTexture :
	public QSGResource
{
public:
	QSGTexture(void) : m_data(NULL), m_width(0), m_height(0), m_components(0) {}
	virtual ~QSGTexture(void);

public:

	// These are public for QSGRenderer implementations, but really they
	// should be moved into an immutable data class that can be sent
	// to the renderer (it will hold a temporary ref)
public:
	unsigned char* m_data;
	int m_width;
	int m_height;
	int m_components;
};
