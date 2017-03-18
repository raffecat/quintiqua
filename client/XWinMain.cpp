#include "global.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
//#include <X11/extensions/Xrandr.h>
#include <GL/glx.h>
#include <sys/time.h>

#include "Logger.h"
#include "LuaController.h"
#include "QSGOpenGLRenderer.h"


// Global Variables
static Display* g_display = NULL;
static int g_screen = 0;
static Window g_window = 0;
static int g_width = 0;
static int g_height = 0;
static XVisualInfo g_visual = {0};
static GLXContext g_context = NULL;
static Atom g_atomClose = 0;
static XIM g_inputMethod = NULL;
static XIC g_inputContext = NULL;
static struct timeval g_lastTime;

static bool m_running = true;
static bool m_active = false;
static bool m_fullscreen = false;
//static bool m_toggleScreen = false;


LuaController* g_controller = 0;
ref_ptr<QSGRenderer> g_renderer;


// Constants
const char *c_mainWindowTitle = "Automata Player";
const char *c_logWindowTitle = "Automata Console";

const char *c_logFilename = "client.log";
const char *c_apiFilename = "api_init.lua";
const char *c_coreFilename = "core.lua";

// Foward declarations
bool CreateMainWindow();
bool CreateLogWindow();
bool CreateGLWindow(int width, int height);
void ResizeGLWindow(int width, int height);
void SwitchToWindowed();
void SwitchToFullscreen();
void DestroyGLWindow();

/*
static int console_write (lua_State *L) {
	log_Log(luaL_checkstring(L, 1));  // throws argument error if not a string.
	return 0;
}

static int quit_host (lua_State *L) {
  // TODO
  return 0;
}

static int SetWindowTitle(lua_State *L) {
	const char* title = luaL_checkstring(L, 1);
	if (g_display && g_window)
		XStoreName(g_display, g_window, title);
	return 0;
}
*/


// ---------------------------------------------------------------------

bool CreateGLContext();
void ReleaseGLContext();
void ResizeGLWindow(int width, int height);
bool RenderGLView();
void UpdateAndRender();


#define cast(X,Y) ((X)(Y))

void EnableVSync(bool enable)
{
    const GLubyte* name = cast(const GLubyte*, "glXSwapIntervalSGI");
    PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI = cast(PFNGLXSWAPINTERVALSGIPROC, glXGetProcAddress(name));
    if (glXSwapIntervalSGI)
		glXSwapIntervalSGI(enable ? 1 : 0);
}

Bool FilterEvents(Display* display, XEvent* ev, XPointer userData)
{
    return ev->xany.window == cast(Window, userData);
}

void ProcessEvent(XEvent ev)
{
    switch (ev.type)
    {
        case ConfigureNotify:
			if ((ev.xconfigure.width != cast(int,g_width)) || (ev.xconfigure.height != cast(int,g_height)))
			{
				g_width  = ev.xconfigure.width;
				g_height = ev.xconfigure.height;

				//log_Logf("ProcessEvent: resize: %d, %d", g_width, g_height);

				ResizeGLWindow(g_width, g_height);
			}
			break;

		case MotionNotify:
			g_controller->mouseMove(ev.xmotion.x, ev.xmotion.y);
			break;

		case ButtonPress:
			{
				int btn = 0;
				if (ev.xbutton.button == Button1) btn = 1; // left
				if (ev.xbutton.button == Button3) btn = 2; // right
				if (ev.xbutton.button == Button2) btn = 3; // middle
				if (btn)
					g_controller->mouseButton(btn, 1);
			}
			break;

		case ButtonRelease:
			{
				int btn = 0;
				if (ev.xbutton.button == Button1) btn = 1;
				if (ev.xbutton.button == Button3) btn = 2;
				if (ev.xbutton.button == Button2) btn = 3;
				if (btn)
					g_controller->mouseButton(btn, 0);
			}
			break;

        case FocusIn:
			log_Log("ProcessEvent: focus in");
			if (g_inputContext)
				XSetICFocus(g_inputContext);
			break;

        case FocusOut:
			log_Log("ProcessEvent: focus out");
			if (g_inputContext)
				XUnsetICFocus(g_inputContext);
			break;

		case MappingNotify:
			// support keyboard mapping changes on the server.
			XRefreshKeyboardMapping(&ev.xmapping);
			break;

		case KeyPress:
			{
				char buffer[32];
				KeySym key;
				int len = XLookupString(&ev.xkey, buffer, sizeof(buffer), &key, NULL);

				// always send a key press event
				g_controller->keyPress(key, 1);

#ifdef X_HAVE_UTF8_STRING
				if (g_inputContext)
				{
					Status status;
					len = Xutf8LookupString(g_inputContext, &ev.xkey, buffer, sizeof(buffer), NULL, &status);
					if (status == XLookupChars)
					{
						g_controller->keyChars(buffer, len);
					}
				}
				else
#endif
				{
					if (len)
					{
						g_controller->keyChars(buffer, len);
					}
				}
			}
			break;

		case KeyRelease:
			{
				char buffer[32];
				KeySym key;
				XLookupString(&ev.xkey, buffer, sizeof(buffer), &key, NULL);
				g_controller->keyPress(key, 0);
			}
			break;

        case ClientMessage:
			if ((ev.xclient.format == 32) && (ev.xclient.data.l[0]) == cast(long,g_atomClose))  
			{
				log_Log("ProcessEvent: close");
				m_running = false;
			}
			break;

        case DestroyNotify:
			log_Log("ProcessEvent: destroy");
			ReleaseGLContext();
			m_running = false;
			break;
	}
}

int main(int argc, char** argv)
{
    const char* disp;
    XSetWindowAttributes attributes;
	Colormap colMap;
    Bool enable = False;
    XEvent ev;

	// create the log file
	log_Open( c_logFilename, NULL );

	if (argc > 1) disp = argv[1];
	else disp = ":0";

	g_display = XOpenDisplay(disp);
    if (!g_display) {
		log_Log("cannot open display");
		return 1;
	}

    g_screen = DefaultScreen(g_display);

    // Disable repetition of KeyRelease events when a key is held down
    XkbSetDetectableAutoRepeat(g_display, True, &enable);
    XFlush(g_display);

    // Get the input method (XIM) object
    g_inputMethod = XOpenIM(g_display, NULL, NULL, NULL);
	if (!g_inputMethod) {
		log_Log("... failed to open input method");
		return false;
	}

    g_width  = 800;
    g_height = 600;

	if (!CreateGLContext()) {
		log_Log("... failed to create GL context");
		return false;
	}

	EnableVSync(true);

    // Create a new color map with the chosen visual
    colMap = XCreateColormap(g_display, RootWindow(g_display, g_screen), g_visual.visual, AllocNone);
	if (!colMap) {
		log_Log("... failed to create colormap");
		return false;
	}

    // Define the window attributes
    attributes.colormap = colMap;
	attributes.event_mask = FocusChangeMask | ButtonPressMask | ButtonReleaseMask | 
                            PointerMotionMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask;

	// Create a dummy window (disabled and hidden)
    g_window = XCreateWindow(g_display,
                             RootWindow(g_display, g_screen),
                             10, 10,
                             g_width, g_height,
                             0,
                             g_visual.depth,
                             InputOutput,
                             g_visual.visual,
                             CWEventMask | CWColormap,
							 &attributes);
	if (!g_window) {
		log_Log("... failed to create window");
		return false;
	}

    g_atomClose = XInternAtom(g_display, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(g_display, g_window, &g_atomClose, 1);

    if (g_inputMethod) {
        g_inputContext = XCreateIC(g_inputMethod,
                                   XNClientWindow, g_window,
                                   XNFocusWindow, g_window,
                                   XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                                   NULL);
        
        if (!g_inputContext)
			log_Log("... failed to create input context for window");
    }

	XAutoRepeatOn(g_display);

    // Show the window
    XMapWindow(g_display, g_window);
    XFlush(g_display);

	// make the rendering context current.
	if (!glXMakeCurrent(g_display, g_window, g_context)) {
		log_Log("CreateGLContext: cannot make rendering context current");
		return false;
	}

    XFlush(g_display);

	// start rendering
	m_active = true;

	g_renderer = new QSGOpenGLRenderer();
	if (!g_renderer)
		return 1;

	g_controller = new LuaController(g_renderer);
	if (!g_controller)
		return 1;

	// init lua and start client
	g_controller->execLua(c_apiFilename);
	g_controller->execLua(c_coreFilename);
	g_controller->resize(g_width, g_height);

	gettimeofday(&g_lastTime, NULL);

	// main message loop
	while (m_running) {
		while (XCheckIfEvent(g_display, &ev, &FilterEvents, cast(XPointer, g_window))) {
			ProcessEvent(ev);
		}

		UpdateAndRender();
	}

    if (g_inputContext)
        XDestroyIC(g_inputContext);

    if (g_window)
        XDestroyWindow(g_display, g_window);

    if (g_inputMethod)
        XCloseIM(g_inputMethod);

    XFlush(g_display);

    XCloseDisplay(g_display);
    g_display = NULL;

	delete g_controller; // manually tracked.
	g_controller = 0;
	g_renderer = 0; // free ref_ptr before main exits.

	log_Close();

	return 0;
}

void UpdateAndRender()
{
	struct timeval tv;
	time_t delta_sec;
	double delta;

	gettimeofday(&tv, NULL);
	delta_sec = tv.tv_sec - g_lastTime.tv_sec;
	if (delta_sec < 0) delta_sec = 0;
	if (delta_sec == 0)
	{
		// still within the same second
		delta = (double)(tv.tv_usec - g_lastTime.tv_usec) * 0.001;
		//printf("delta: %g\n", (double)delta);
	}
	else
	{
		// rolled over into a new second one or more times
		delta = (double)(1000000 - g_lastTime.tv_usec) * 0.001 +
			    (double)tv.tv_usec * 0.001 +
				(double)(delta_sec - 1) * 1000;

		//printf("delta: %ld %ld %ld %g\n", (long)delta_sec,
		//	(long)(1000000 - g_lastTime.tv_usec),
		//	(long)tv.tv_usec, (double)delta);
	}
	g_lastTime = tv;

	g_controller->update(delta);

	RenderGLView();
}



/* ----------------------------------------------------------------------- */
/* OpenGL view */

#include <GL/gl.h>
//#include <GL/glu.h>


void ResizeGLWindow(int width, int height)
{
	if (m_active && !m_fullscreen)
	{
		if (g_context && width > 0 && height > 0)
		{
			g_controller->resize(width, height);
		}
	}
}

bool RenderGLView()
{
	if (!g_context)
	{
		log_Log("RenderGLView: no rendering context");
		return false;
	}

	// shown, not minimized, visible region?

	g_controller->render();

	// commit all drawing commands.
	// glFinish();

	// swap the window buffers.
	glXSwapBuffers(g_display, g_window);

	return true;
}

bool CreateGLContext()
{
    int numVisuals = 0;
	XVisualInfo* visuals;
	int rgba, doubleBuffer;

    g_visual.screen = g_screen;
    visuals = XGetVisualInfo(g_display, VisualScreenMask, &g_visual, &numVisuals);
    if (!visuals || !numVisuals)
    {
        if (visuals)
            XFree(visuals);
		log_Log("CreateGLContext: no visuals available");
        return false;
    }

	g_visual = visuals[0];
	XFree(visuals);

    glXGetConfig(g_display, &g_visual, GLX_RGBA, &rgba);
    glXGetConfig(g_display, &g_visual, GLX_DOUBLEBUFFER, &doubleBuffer);

	if (!rgba || !doubleBuffer) {
		log_Log("CreateGLContext: requires rgba and double-buffer");
        return false;
	}

	// create the GL rendering context.
    g_context = glXCreateContext(g_display, &g_visual, glXGetCurrentContext(), true);
	if (!g_context) {
		log_Log("CreateGLContext: cannot create rendering context");
		return false;
	}

	return true;
}

void ReleaseGLContext()
{
	glXMakeCurrent(g_display, None, NULL);

	if (g_context)
		glXDestroyContext(g_display, g_context);

	g_context = NULL;
}
