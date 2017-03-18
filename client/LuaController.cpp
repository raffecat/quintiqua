#include "LuaController.h"
#include "QSGRenderer.h"
#include "QSGViewport.h"
#include "QSGTransformNode.h"
#include "QSGFrame.h"
#include "QSGClipView.h"
#include "QSGTexture.h"
#include "QSGGraphic.h"
#include "Logger.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "luasocket.h"
#include "xlua.h"
#include "stb_image.h"
}

static int registerLuaFuncs(lua_State* L);
static int printToConsole(lua_State* L);
static int quitApplication(lua_State* L);
static int setWindowTitle(lua_State* L);
static int report(lua_State *L, int status);

LuaController::LuaController(QSGRenderer* renderer)
{
	// Create Lua states
	m_lua = luaL_newstate();

	// Open all libs on the system state
	luaL_openlibs(m_lua);

	// Open the luasocket lib
	report(m_lua, lua_cpcall(m_lua, luaopen_socket_core, 0));

	report(m_lua, lua_cpcall(m_lua, registerLuaFuncs, 0));

	m_viewport = new QSGViewport();

	m_renderer = renderer;
}

LuaController::~LuaController()
{
	lua_close(m_lua);
	m_lua = NULL;
}

void LuaController::resize(int width, int height)
{
	m_renderer->setViewportSize(width, height);
	lua_State* L = this->m_lua;
	lua_pushcfunction(L, xlua_traceback);
	lua_getglobal(L, "sg_size_change");
	lua_pushnumber(L, width);
	lua_pushnumber(L, height);
	int result = lua_pcall(L, 2, 0, -4);
	if (result) lua_remove(L, -2); // xlua_traceback
	else lua_pop(L, 1); // xlua_traceback
	report(L, result);
}

void LuaController::mouseMove(int x, int y)
{
	lua_State* L = this->m_lua;
	lua_pushcfunction(L, xlua_traceback);
	lua_getglobal(L, "sg_mouse_move");
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	int result = lua_pcall(L, 2, 0, -4);
	if (result) lua_remove(L, -2); // xlua_traceback
	else lua_pop(L, 1); // xlua_traceback
	report(L, result);
}

void LuaController::mouseButton(int button, int down)
{
	lua_State* L = this->m_lua;
	lua_pushcfunction(L, xlua_traceback);
	lua_getglobal(L, "sg_mouse_button");
	lua_pushnumber(L, button);
	lua_pushnumber(L, down);
	int result = lua_pcall(L, 2, 0, -4);
	if (result) lua_remove(L, -2); // xlua_traceback
	else lua_pop(L, 1); // xlua_traceback
	report(L, result);
}

void LuaController::keyPress(int key, int down)
{
	lua_State* L = this->m_lua;
	lua_pushcfunction(L, xlua_traceback);
	lua_getglobal(L, "sg_key_press");
	lua_pushnumber(L, key);
	lua_pushnumber(L, down);
	int result = lua_pcall(L, 2, 0, -4);
	if (result) lua_remove(L, -2); // xlua_traceback
	else lua_pop(L, 1); // xlua_traceback
	report(L, result);
}

void LuaController::keyChars(char* bytes, int len)
{
	lua_State* L = this->m_lua;
	lua_pushcfunction(L, xlua_traceback);
	lua_getglobal(L, "sg_key_char");
	lua_pushlstring(L, bytes, len);
	int result = lua_pcall(L, 1, 0, -3);
	if (result) lua_remove(L, -2); // xlua_traceback
	else lua_pop(L, 1); // xlua_traceback
	report(L, result);
}

bool LuaController::execLua(const char* filename)
{
	lua_State* L = this->m_lua;
	lua_pushcfunction(L, xlua_traceback); // push traceback function
	if (!report(L, luaL_loadfile(L, filename))) {
		report(L, lua_pcall(L, 0, 0, -2));
	}
	lua_pop(L, 1); // pop traceback function
	return true;
}

void LuaController::update(double delta)
{
	lua_State* L = this->m_lua;
	lua_pushcfunction(L, xlua_traceback);
	lua_getglobal(L, "sg_update");
	lua_pushnumber(L, delta);
	int result = lua_pcall(L, 1, 0, -3);
	if (result) lua_remove(L, -2); // xlua_traceback
	else lua_pop(L, 1); // xlua_traceback
	report(L, result);
}

bool LuaController::render(void)
{
	m_renderer->render(m_viewport);
	return true;
}

int LuaController::createLuaObject(QSGObject* obj)
{
	obj->retain(); // hold a ref for lua
	m_objects.insert(obj); // add to set of valid objects
	lua_pushlightuserdata(m_lua, obj); // push u
	return 1; // return u
}

void LuaController::destroyLuaObject(QSGObject* obj)
{
	// Lua can have many uncounted refs to the object, so check if the
	// object still exists before trying to access it.
	if (m_objects.find(obj) != m_objects.end())
	{
		// Remove from the index of lua objects and
		// drop the ref we were keeping for lua.
		m_objects.erase(obj);
		obj->release();
	}
}

QSGObject* LuaController::checkObject(int index)
{
	QSGObject* obj = reinterpret_cast<QSGObject*>(lua_touserdata(m_lua, index));
	if (m_objects.find(obj) != m_objects.end()) return obj;
	else {
		luaL_error(m_lua, "argument is not a scene-graph object");
		return NULL; // never reached.
	}
}

template<class T>
T* LuaController::toObject(int index)
{
	QSGObject* obj = checkObject(index);
	T* node = dynamic_cast<T*>(obj);
	if (node) return node;
	else {
		luaL_error(m_lua, "wrong type of scene-graph object");
		return NULL; // never reached.
	}
}

int printToConsole (lua_State *L) {
	const char* msg = luaL_checkstring(L, 1);
	log_Log(msg);  // throws argument error if not a string.
	return 0;
}

int quitApplication (lua_State *L)
{
	//PostQuitMessage(0);
	return 0;
}

int setWindowTitle(lua_State *L)
{
	const char* title = luaL_checkstring(L, 1);
	//SetWindowText(m_mainWnd, title);
	return 0;
}

int create_transform_node(lua_State *L) {
	return g_controller->createLuaObject(new QSGTransformNode());
}

int create_frame(lua_State *L) {
	return g_controller->createLuaObject(new QSGFrame());
}

int create_clip(lua_State *L) {
	return g_controller->createLuaObject(new QSGClipView());
}

int create_graphic(lua_State *L) {
	return g_controller->createLuaObject(new QSGGraphic());
}

int set_parent(lua_State *L) {
	QSGNode* node = g_controller->toObject<QSGNode>(1);
	if (lua_isnoneornil(L, 2)) {
		// remove from current parent.
		if (node->getParent()) {
			node->getParent()->removeChild(node);
		}
	}
	else {
		// move from current parent to new parent.
		QSGNode* parent = g_controller->toObject<QSGNode>(2);
		parent->appendChild(node);
	}
	return 0;
}

int set_position(lua_State *L) {
	QSGTransformNode* node = g_controller->toObject<QSGTransformNode>(1);
	float x = (float) lua_tonumber(L, 2);
	float y = (float) lua_tonumber(L, 3);
	node->m_transform.pos = QSGVec2(x, y);
	return 0;
}

int set_angle(lua_State *L) {
	QSGTransformNode* node = g_controller->toObject<QSGTransformNode>(1);
	float a = (float) lua_tonumber(L, 2);
	node->m_transform.angle = a;
	return 0;
}

int set_scale(lua_State *L) {
	QSGTransformNode* node = g_controller->toObject<QSGTransformNode>(1);
	float x = (float) lua_tonumber(L, 2);
	float y = (float) lua_tonumber(L, 3);
	node->m_transform.scale = QSGVec2(x, y);
	return 0;
}

int set_colour(lua_State *L) {
	QSGTransformNode* node = g_controller->toObject<QSGTransformNode>(1);
	float r = (float) lua_tonumber(L, 2);
	float g = (float) lua_tonumber(L, 3);
	float b = (float) lua_tonumber(L, 4);
	float a = (float) luaL_optnumber(L, 5, 1);
	node->m_transform.col = QSGColour(r, g, b, a);
	return 0;
}

int set_outline(lua_State *L) {
	QSGTransformNode* node = g_controller->toObject<QSGTransformNode>(1);
	return 0;
}

const char* blendModes[] = {
	"modulate",
	"add",
	NULL
};

int set_blend_mode(lua_State *L) {
	QSGTransformNode* node = g_controller->toObject<QSGTransformNode>(1);
	if (luaL_checkoption(L, 2, NULL, blendModes) == 1)
		node->m_transform.flags |= QSGTransformBlendAdd;
	else
		node->m_transform.flags &= ~QSGTransformBlendAdd;
	return 0;
}

int frame_set_shape(lua_State *L) {
	QSGFrame* node = g_controller->toObject<QSGFrame>(1);
	float left = (float) lua_tonumber(L, 2);
	float bottom = (float) lua_tonumber(L, 3);
	float right = (float) lua_tonumber(L, 4);
	float top = (float) lua_tonumber(L, 5);
	node->m_left = left;
	node->m_bottom = bottom;
	node->m_right = right;
	node->m_top = top;
	return 0;
}

int clip_set_shape(lua_State *L) {
	QSGClipView* node = g_controller->toObject<QSGClipView>(1);
	float left = (float) lua_tonumber(L, 2);
	float bottom = (float) lua_tonumber(L, 3);
	float right = (float) lua_tonumber(L, 4);
	float top = (float) lua_tonumber(L, 5);
	node->m_left = left;
	node->m_bottom = bottom;
	node->m_right = right;
	node->m_top = top;
	return 0;
}

int frame_set_texture(lua_State *L) {
	QSGFrame* node = g_controller->toObject<QSGFrame>(1);
	QSGTexture* tex = g_controller->toObject<QSGTexture>(2);
	node->m_texture = tex;
	// force blend mode if the texture contains alpha.
	if (tex->m_components == 4) node->m_transform.flags |= QSGTransformNeedsBlend;
	else node->m_transform.flags &= ~QSGTransformNeedsBlend;
	return 0;
}

int graphic_set_texture(lua_State *L) {
	QSGGraphic* node = g_controller->toObject<QSGGraphic>(1);
	QSGTexture* tex = g_controller->toObject<QSGTexture>(2);
	node->m_texture = tex;
	// force blend mode if the texture contains alpha.
	if (tex->m_components == 4) node->m_transform.flags |= QSGTransformNeedsBlend;
	else node->m_transform.flags &= ~QSGTransformNeedsBlend;
	return 0;
}

int graphic_set_geometry(lua_State *L) {
	QSGGraphic* node = g_controller->toObject<QSGGraphic>(1);
	luaL_checktype(L, 2, LUA_TTABLE); // indices
	luaL_checktype(L, 3, LUA_TTABLE); // verts
	luaL_checktype(L, 4, LUA_TTABLE); // coords
	int nindices = luaL_getn(L, 2);
	int nverts = luaL_getn(L, 3);
	int ncoords = luaL_getn(L, 4);
	// determine number of valid vertices
	int numvalid = nverts / 2;
	if (ncoords && ncoords < nverts) numvalid = ncoords / 2;
	if (numvalid > 65535) luaL_error(L, "too many vertices");
	// assign indices
	QSGGeometry::indicesType& indices = node->m_geometry.indices;
	indices.clear();
	indices.reserve(nindices);
	for (int i = 1; i <= nindices; ++i) {
		lua_rawgeti(L, 2, i);
		lua_Integer idx = lua_tointeger(L, -1);
		if (idx < 0 || idx >= numvalid) luaL_error(L,
			"index %d out of range (%d valid vertices)", (int)idx, numvalid);
		indices.push_back( (unsigned short) idx );
		lua_pop(L, 1);
	}
	// assign vertices
	QSGGeometry::verticesType& verts = node->m_geometry.verts;
	nverts = numvalid * 2;
	verts.clear();
	verts.reserve(nverts);
	for (int i = 1; i <= nverts; ++i) {
		lua_rawgeti(L, 3, i);
		lua_Number val = lua_tonumber(L, -1);
		verts.push_back((float)val);
		lua_pop(L, 1);
	}
	// assign texture coords
	if (ncoords)
	{
		QSGGeometry::verticesType& coords = node->m_geometry.coords;
		ncoords = numvalid * 2;
		coords.clear();
		coords.reserve(ncoords);
		for (int i = 1; i <= nverts; ++i) {
			lua_rawgeti(L, 4, i);
			lua_Number val = lua_tonumber(L, -1);
			coords.push_back((float)val);
			lua_pop(L, 1);
		}
	}
	node->m_geometry.quads = true;
	return 0;
}

int load_texture(lua_State *L) {
	const char* filename = luaL_checklstring(L, 1, NULL);
	int width, height, comp;
	stbi_uc* data = stbi_load(filename, &width, &height, &comp, STBI_default);
	if (!data) {
		luaL_error(L, "load failed: %s (%s)", filename, stbi_failure_reason());
	}
	/* zero out texels that have zero alpha. this will break clever shader
	// tricks, but we can't have white/garbage pixels adjacent to blended
	// pixels otherwise the garbage will bleed into the image.
	if (comp == 4) {
		size_t len = width * height * 4;
		for (size_t i = 0; i < len; i += 4) {
			if (data[i+3] == 0) {
				data[i] = data[i+1] = data[i+2] = 0;
			}
		}
	}*/
	QSGTexture* tex = new QSGTexture();
	tex->m_data = data;
	tex->m_width = width;
	tex->m_height = height;
	tex->m_components = comp;
	return g_controller->createLuaObject(tex);
}

int get_tetxure_size(lua_State *L) {
	QSGTexture* tex = g_controller->toObject<QSGTexture>(1);
	lua_pushnumber(L, tex->m_width);
	lua_pushnumber(L, tex->m_height);
	lua_pushnumber(L, tex->m_components);
	return 3;
}

int viewport_set_bg(lua_State *L) {
	float r = (float) lua_tonumber(L, 1);
	float g = (float) lua_tonumber(L, 2);
	float b = (float) lua_tonumber(L, 3);
	g_controller->m_viewport->setBackground(QSGColour(r, g, b));
	return 0;
}

int viewport_set_scene(lua_State *L) {
	QSGNode* scene = g_controller->toObject<QSGNode>(1);
	g_controller->m_viewport->removeAllChildren();
	g_controller->m_viewport->appendChild(scene);
	return 0;
}

static int sg_destroy(lua_State* L)
{
	QSGObject* obj = reinterpret_cast<QSGObject*>(lua_touserdata(L, 1));
	g_controller->destroyLuaObject(obj);
	return 0;
}

static const luaL_Reg sg_methods[] = {
	{"createTransform", create_transform_node},
	{"createFrame", create_frame},
	{"createClip", create_clip},
	{"createGraphic", create_graphic},
	{"setParent", set_parent},
	{"setPosition", set_position},
	{"setAngle", set_angle},
	{"setScale", set_scale},
	{"setColour", set_colour},
	{"setShape", frame_set_shape},
	{"setClipShape", clip_set_shape},
	{"setTexture", frame_set_texture},
	{"setGfxTexture", graphic_set_texture},
	{"setGeometry", graphic_set_geometry},
	{"loadTexture", load_texture},
	{"getTextureSize", get_tetxure_size},
	{"setOutline", set_outline},
	{"setBlendMode", set_blend_mode},
	{"setBackground", viewport_set_bg},
	{"setScene", viewport_set_scene},
	{"destroy", sg_destroy},
	{NULL, NULL}
};

static int encode(lua_State *L) {
	luaL_Buffer b;
	const char* fmt = luaL_checkstring(L, 1);
	luaL_buffinit(L, &b);
	int arg = 2;
	for(;;)
	{
		switch (*fmt)
		{
		case '\0': {
			luaL_pushresult(&b);
			return 1;
		}
		case 'B':
		case 'b': {
			lua_Integer v = lua_tointeger(L, arg);
			luaL_addchar(&b, v);
			break;
		}
		case 'H':
		case 'h': {
			lua_Integer v = lua_tointeger(L, arg);
			luaL_addchar(&b, (v >> 8));
			luaL_addchar(&b, v);
			break;
		}
		case 'I':
		case 'i': {
			lua_Integer v = lua_tointeger(L, arg);
			luaL_addchar(&b, (v >> 24));
			luaL_addchar(&b, (v >> 16));
			luaL_addchar(&b, (v >> 8));
			luaL_addchar(&b, v);
			break;
		}
		case 'S': {
			size_t len = 0;
			const char* data = lua_tolstring(L, arg, &len);
			if (len > 65535) len = 65535;
			luaL_addchar(&b, (len >> 8));
			luaL_addchar(&b, len);
			luaL_addlstring(&b, data, len);
			break;
		}
		case 's': {
			size_t len = 0;
			const char* data = lua_tolstring(L, arg, &len);
			if (len > 255) len = 255;
			luaL_addchar(&b, len);
			luaL_addlstring(&b, data, len);
			break;
		}
		default:
			luaL_error(L, "unknown format specifier '%c' in encode", *fmt);
		}
		++arg;
		++fmt;
	}
}

static int decode(lua_State *L) {
	size_t nargs = 0;
	const char* fmt = luaL_checklstring(L, 1, &nargs);
	// make sure nargs is within (int) range.
	if (nargs > LUAI_MAXCSTACK) nargs = LUAI_MAXCSTACK+1;
	lua_checkstack(L, (int)nargs);
	size_t size = 0;
	const unsigned char* data = (const unsigned char*) luaL_checklstring(L, 2, &size);
	size_t datasize = size;
	for (;;)
	{
		switch (*fmt)
		{
		case '\0':
			return (int)nargs;
		case 'B': {
			if (size < 1) break;
			lua_pushinteger(L, data[0]);
			data += 1;
			size -= 1;
			break;
		}
		case 'b': {
			if (size < 1) break;
			lua_pushinteger(L, (signed char) data[0]);
			data += 1;
			size -= 1;
			break;
		}
		case 'H': {
			if (size < 2) break;
			lua_Integer v = ((lua_Integer) data[0]) << 8;
			v |= data[1];
			lua_pushinteger(L, v);
			data += 2;
			size -= 2;
			break;
		}
		case 'h': {
			if (size < 2) break;
			lua_Integer v = ((lua_Integer)(signed char)data[0]) << 8;
			v |= data[1];
			lua_pushinteger(L, v);
			data += 2;
			size -= 2;
			break;
		}
		case 'I': {
			if (size < 4) break;
			unsigned long v = ((unsigned long)data[0]) << 24;
			v |= ((unsigned long)data[1]) << 16;
			v |= ((unsigned long)data[2]) << 8;
			v |= data[3];
			lua_pushnumber(L, v);
			data += 4;
			size -= 4;
			break;
		}
		case 'i': {
			if (size < 4) break;
			lua_Integer v = ((lua_Integer)(signed char)data[0]) << 24;
			v |= ((lua_Integer)data[1]) << 16;
			v |= ((lua_Integer)data[2]) << 8;
			v |= data[3];
			lua_pushinteger(L, v);
			data += 4;
			size -= 4;
			break;
		}
		case 'S': {
			if (size < 2) break;
			size_t len = ((size_t) data[0]) << 8;
			len |= data[1];
			if (size - 2 < len) break;
			lua_pushlstring(L, (const char*)(data + 2), len);
			data += 2 + len;
			size -= 2 + len;
			break;
		}
		case 's': {
			if (size < 1) break;
			size_t len = data[0];
			if (size - 1 < len) break;
			lua_pushlstring(L, (const char*)(data + 1), len);
			data += 1 + len;
			size -= 1 + len;
			break;
		}
		default:
			luaL_error(L, "unknown format specifier '%c' in decode", *fmt);
		}
		fmt++;
	}
	luaL_error(L, "data truncated at offset  '%c' in decode", *fmt);
}

int registerLuaFuncs(lua_State *L)
{
	lua_register(L, "print", printToConsole);
	lua_register(L, "quit", quitApplication);
	lua_register(L, "encode", encode);
	lua_register(L, "decode", decode);
	lua_register(L, "SetWindowTitle", setWindowTitle);
	luaL_register(L, "sg", sg_methods);
	return 0;
}

int report(lua_State *L, int status)
{
  if (status) {
    /* -1 is error message from xlua_traceback */
	if (luaL_checkstring(L, -1)) {
		const char *msg = lua_tostring(L, -1);
		if (msg) {
			log_Log(msg);
			//ShowLogWindow();
		}
		lua_pop(L, 1); /* pop error message */
	}
  }
  return status;
}
