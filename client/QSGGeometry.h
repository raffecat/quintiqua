#pragma once
#include "QSGResource.h"
#include <vector>


class QSGGeometry :
	public QSGResource
{
public:
	QSGGeometry(void) : quads(false) {}

public:
	typedef std::vector<float> verticesType;
	typedef std::vector<unsigned short> indicesType;

public:
	verticesType verts;
	verticesType coords;
	indicesType indices;
	bool quads;
};
