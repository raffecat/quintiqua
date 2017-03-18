#include "QSGOpenGLRenderer.h"
#include "QSGTransform.h"
#include "QSGTexture.h"
#include "QSGGeometry.h"
#include "QSGNode.h"

QSGOpenGLRenderer::~QSGOpenGLRenderer(void)
{
}

void QSGOpenGLRenderer::initialise(void)
{
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClearDepth( 1.0f );
	glDisable( GL_DEPTH_TEST ); // for now.

	//glDepthFunc( GL_LEQUAL );
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST ); // for now.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	//glFrontFace( GL_CCW );
	glDisable( GL_CULL_FACE ); // for now.

	// glEnable(GL_MULTISAMPLE_ARB); // TODO
}

void QSGOpenGLRenderer::shutdown(void)
{
}

void QSGOpenGLRenderer::setViewportSize(int width, int height)
{
	m_width = width;
	m_height = height;

	if (width > 0 && height > 0)
	{
		float w = width * 0.5f;
		float h = height * 0.5f;
		glViewport(0, 0, (GLsizei)width, (GLsizei)height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-w, w, -h, h, 1, -1);
		glMatrixMode(GL_MODELVIEW);
	}
}

void QSGOpenGLRenderer::render(QSGNode* scene)
{
	scene->render(this);
}

void QSGOpenGLRenderer::clear(QSGColour colour)
{
	glClearColor( colour.r, colour.g, colour.b, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT);

	glLoadIdentity();
}

void QSGOpenGLRenderer::pushTransform(QSGTransform* trans)
{
	// TODO: avoid matrix stack
	glPushMatrix();

	// TODO: cumulative colour change
	glColor4f(trans->col.r, trans->col.g, trans->col.b, trans->col.a);

	// Enable blending if:
	// - the colour alpha is not opaque, or
	// - the texture has an alpha channel, or
	// - blend mode is additive (might be a luminance texture)
	if (trans->col.a != 1 || trans->flags & (QSGTransformNeedsBlend | QSGTransformBlendAdd)) {
		if (!m_blending) {
			glEnable(GL_BLEND);
			m_blending = true;
		}
		if (trans->flags & QSGTransformBlendAdd) {
			if (!m_additive) {
				glBlendFunc(GL_ONE, GL_ONE);
				m_additive = true;
			}
		}
		else {
			if (m_additive) {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				m_additive = false;
			}
		}
	}
	else {
		if (m_blending) {
			glDisable(GL_BLEND);
			m_blending = false;
		}
	}

	// Apply SRT transform.
	glTranslatef(trans->pos.x, trans->pos.y, 0);
	if( trans->angle > 0.00001 || trans->angle < -0.00001 ) {
		glRotatef( trans->angle, 0, 0, 1); // rotate around origin
	}
	glScalef(trans->scale.x, trans->scale.y, 0);
}

void QSGOpenGLRenderer::popTransform()
{
	glPopMatrix();
}

void QSGOpenGLRenderer::setTexture(class QSGTexture* texture)
{
	if (!m_texturing) {
		glEnable(GL_TEXTURE_2D);
		m_texturing = true;
	}
	if (texture->m_renderData)
	{
		glBindTexture(GL_TEXTURE_2D, texture->m_renderData);
	}
	else
	{
		resolveTexture(texture);
	}
}

void QSGOpenGLRenderer::clearTexture(void)
{
	if (m_texturing) {
		glDisable(GL_TEXTURE_2D);
		m_texturing = false;
	}
}

void QSGOpenGLRenderer::renderQuad(float left, float bottom, float right, float top)
{
	static GLfloat coords[8] = { 0, 1, 1, 1, 1, 0, 0, 0 };
	GLfloat verts[8] = {
		left, bottom, right, bottom,
		right, top, left, top
	};

	if (!m_arrays) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		m_arrays = true;
	}

	glTexCoordPointer(2, GL_FLOAT, 0, coords);
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glDrawArrays(GL_QUADS, 0, 4);
}

void QSGOpenGLRenderer::renderGeometry(QSGGeometry* geometry)
{
	GLsizei count = (GLsizei) geometry->indices.size();
	if (count > 0)
	{
		GLfloat* verts = & geometry->verts[0];
		GLushort* indices = & geometry->indices[0];

		if (!m_arrays) {
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			m_arrays = true;
		}

		if (geometry->coords.size()) {
			GLfloat* coords = & geometry->coords[0];
			glTexCoordPointer(2, GL_FLOAT, 0, coords);
		}

		glVertexPointer(2, GL_FLOAT, 0, verts);

		GLenum mode = geometry->quads ? GL_QUADS : GL_TRIANGLES;
		glDrawElements(mode, count, GL_UNSIGNED_SHORT, indices);
	}
}

void QSGOpenGLRenderer::setScissor(float left, float bottom, float right, float top)
{
	GLfloat matrix[16], tx, ty;
	GLint i_left, i_bottom, i_right, i_top;

	// get current transform origin in screen space (hax)
	glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
	tx = this->m_width * 0.5f + matrix[3];
	ty = this->m_height * 0.5f + matrix[7];

	i_left = (GLint) (tx + left);
	i_right = (GLint) (tx + right);
	i_bottom = (GLint) (ty + bottom);
	i_top = (GLint) (ty + top);

	if (i_left < 0) i_left = 0;
	if (i_bottom < 0) i_bottom = 0;
	if (i_right > this->m_width) i_right = this->m_width;
	if (i_top > this->m_height) i_top = this->m_height;
	glScissor(i_left, i_bottom, i_right - i_left, i_top - i_bottom);

	glEnable(GL_SCISSOR_TEST);
}

void QSGOpenGLRenderer::clearScissor(void)
{
	glDisable(GL_SCISSOR_TEST);
}

void QSGOpenGLRenderer::resolveTexture(QSGTexture* texture)
{
	GLuint id;
	glGenTextures(1, &id);
	texture->m_renderData = id;

	GLenum fmt;
	switch (texture->m_components)
	{
	case 1: fmt = GL_LUMINANCE; break; // or GL_ALPHA?
	case 2: fmt = GL_LUMINANCE_ALPHA; break;
	case 3: fmt = GL_RGB; break;
	case 4: fmt = GL_RGBA; break;
	default: /* invalid */ return;
	}

	// If this is ever needed on any platform, make sure row alignment is
	// one byte for RGB images since we don't pad texel row data.
	GLint align;
	if (texture->m_components == 3) align = 1;
	else align = texture->m_components;

	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, align);
	glTexImage2D(GL_TEXTURE_2D, 0, texture->m_components,
		texture->m_width, texture->m_height, 0, fmt,
		GL_UNSIGNED_BYTE, texture->m_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ); // GL_CLAMP | GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ); // GL_CLAMP | GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ); // GL_NEAREST | GL_LINEAR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); // GL_NEAREST | GL_LINEAR
}
