#pragma once
#include "QSGObject.h"
#include "QSGTransform.h" // QSGColour

class QSGNode;
class QSGTransform;
class QSGTexture;
class QSGGeometry;

// Interface to a rendering implementation.
class QSGRenderer :
	public QSGObject
{
public:
	virtual ~QSGRenderer(void) {}

	// This is the interface to the renderer from outside, i.e. the thing
	// that owns the drawing surface and wants to render the scene.
public:
	// Must be called before rendering.
	virtual void initialise(void) = 0;
	virtual void shutdown(void) = 0;

	// Can be called after initialise but not during rendering.
	virtual void setViewportSize(int width, int height) = 0;

	// Can be called after initialise.
	virtual void render(QSGNode* scene) = 0;


	// This is the interface used by scene graph elements to draw
	// their content when this renderer visits the graph.
public:
	// Clear the viewport.
	virtual void clear(QSGColour clearColour) = 0;

	// Apply a 2D scale-rotate-translate-colour transform.
	virtual void pushTransform(QSGTransform* transform) = 0;

	// Undo the previous transform (deprecated)
	virtual void popTransform(void) = 0;

	// Make this texture the active texture.
	virtual void setTexture(QSGTexture* texture) = 0;

	// Clear the active texture - stop texturing.
	virtual void clearTexture(void) = 0;

	// Render an axis-aligned 2D quadrilateral.
	virtual void renderQuad(float left, float bottom, float right, float top) = 0;

	// Render an indexed geometry buffer.
	virtual void renderGeometry(QSGGeometry* geometry) = 0;

	// Set scissor clip quadrilateral.
	virtual void setScissor(float left, float bottom, float right, float top) = 0;

	// Clear scissor clip.
	virtual void clearScissor(void) = 0;
};

// Base class for implementation-specific renderer data stored in resources.
// These are attached to subclasses of QSGResource by the renderer.
class QSGRenderData
{
};
