#pragma once
#include <set>
#include "QSGObject.h"

struct lua_State;
class QSGViewport;
class QSGRenderer;
class QSGNode;
class QSGTransformNode;
class QSGFrame;

class LuaController
{
public:
	LuaController(QSGRenderer* renderer);
	~LuaController(void);

public:
	void resize(int width, int height);
	bool execLua(const char* filename);
	void update(double delta);
	bool render(void);
	void mouseMove(int x, int y);
	void mouseButton(int button, int down);
	void keyPress(int key, int down);
	void keyChars(char* bytes, int len);

public: // internal
	int createLuaObject(QSGObject* obj);
	void destroyLuaObject(QSGObject* obj);
	QSGObject* checkObject(int index);
	template<class T> T* toObject(int index);

protected:
	void log(const char* message);

public: // for lua calls
	ref_ptr<QSGViewport> m_viewport;

protected:
	struct lua_State* m_lua;
	ref_ptr<QSGRenderer> m_renderer;
	std::set<QSGObject*> m_objects;
};

// hax, so lua can find the controller.
extern LuaController* g_controller;
