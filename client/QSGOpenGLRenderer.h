#pragma once
#include "QSGRenderer.h"

#ifdef WINDOWS
#include <windows.h> // for gl.
#else
#include <X11/Xlib.h>
#endif

#include <GL/gl.h>

#ifndef GL_CLAMP_TO_EDGE /* sigh */
#define GL_CLAMP_TO_EDGE 0x812F
#endif

class QSGOpenGLRenderer :
	public QSGRenderer
{
public:
	QSGOpenGLRenderer(void) : m_width(0), m_height(0),
		m_texturing(false), m_blending(false), m_additive(false),
		m_arrays(false) {}
	virtual ~QSGOpenGLRenderer(void);

public:
	virtual void initialise(void);
	virtual void shutdown(void);
	virtual void setViewportSize(int width, int height);
	virtual void render(QSGNode* scene);

public:
	virtual void clear(QSGColour clearColour);
	virtual void pushTransform(QSGTransform* trans);
	virtual void popTransform(void);
	virtual void setTexture(QSGTexture* texture);
	virtual void clearTexture(void);
	virtual void renderQuad(float left, float bottom, float right, float top);
	virtual void renderGeometry(QSGGeometry* geometry);
	virtual void setScissor(float left, float bottom, float right, float top);
	virtual void clearScissor(void);

protected:
	void resolveTexture(QSGTexture* texture);

protected:
	int m_width;
	int m_height;
	bool m_texturing;
	bool m_blending;
	bool m_additive;
	bool m_arrays;
};
