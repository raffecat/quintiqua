#pragma once

enum QSGTransformFlags {
	QSGTransformNone = 0,
	QSGTransformNeedsBlend = 1,
	QSGTransformBlendAdd = 2,
};

class QSGColour
{
public:
	QSGColour() : r(0), g(0), b(0), a(0) {}
	QSGColour(float r_, float g_, float b_, float a_=1) : r(r_), g(g_), b(b_), a(a_) {}
public:
	float r, g, b, a;
};

class QSGVec2
{
public:
	QSGVec2() : x(0), y(0) {}
	QSGVec2(float x_, float y_) : x(x_), y(y_) {}
public:
	float x, y;
};

extern QSGVec2 QSGOrigin;
extern QSGVec2 QSGNoScale;
extern QSGColour QSGBlack;
extern QSGColour QSGWhite;
extern QSGColour QSGMidtone;

class QSGTransform
{
public:
	QSGTransform() :
	    flags(QSGTransformNone), pos(QSGOrigin),
		angle(0), scale(QSGNoScale), col(QSGWhite)
	{
	}

public:
	unsigned int flags;
	QSGVec2 pos;
	float angle;
	QSGVec2 scale;
	QSGColour col;
};
