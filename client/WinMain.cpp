#include "global.h"

#include <windows.h>
#include <mmsystem.h> // timeGetTime

#include "Logger.h"
#include "LuaController.h"
#include "QSGOpenGLRenderer.h"


// Global Variables
static HINSTANCE m_hInstance;
static HWND m_mainWnd, m_glWnd, m_logWnd, m_logBox;
static BOOL m_active = FALSE;
static BOOL m_fullscreen = FALSE;
static RECT m_rectWindow;
static DWORD g_lastTime;
static bool g_logVisible = false;

static bool g_useTimer = false;

LuaController* g_controller = 0;
ref_ptr<QSGRenderer> g_renderer;

// Constants
const char *c_mainWindowClass = "QuintiquaMainWindow";
const char *c_logWindowClass = "QuintiquaLogWindow";
const char *c_glWindowClass = "QuintiquaGLWindow";

const char *c_mainWindowTitle = "Quintiqua";
const char *c_logWindowTitle = "Quintiqua Console";

const char *c_logFilename = "client.log";
const char *c_apiFilename = "api_init.lua";
const char *c_coreFilename = "core.lua";

#define IDM_OPEN_LOG (WM_APP+20)

// Foward declarations
BOOL RegisterMainClass(HINSTANCE);
BOOL RegisterLogClass(HINSTANCE);
BOOL RegisterGLClass(HINSTANCE);

BOOL CreateMainWindow();
BOOL CreateLogWindow();
BOOL CreateGLWindow(int width, int height);
void ResizeGLWindow(int width, int height);
void SwitchToWindowed();
void SwitchToFullscreen();
void DestroyGLWindow();
void UpdateAndRender();

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GLWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LogWndProc(HWND, UINT, WPARAM, LPARAM);

void ShowLogWindow();



// ---------------------------------------------------------------------


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	RECT rectClient;

	m_hInstance = hInstance;

	// register window classes
	if (!RegisterMainClass(hInstance)) return FALSE;
	if (!RegisterLogClass(hInstance)) return FALSE;
	if (!RegisterGLClass(hInstance)) return FALSE;

	// create main window
	if (!CreateLogWindow()) return FALSE;
	if (!CreateMainWindow()) return FALSE;

	// add the log window to the menu
	HMENU sysMenu = GetSystemMenu(m_mainWnd, FALSE);
	InsertMenu(sysMenu, SC_CLOSE, MF_BYCOMMAND | MF_STRING, IDM_OPEN_LOG, "Open Log Window");
	InsertMenu(sysMenu, SC_CLOSE, MF_BYCOMMAND | MF_SEPARATOR, 0, "");
	DrawMenuBar(m_mainWnd); // must call to update cached data.

	// create the log file
	log_Open( c_logFilename, m_logBox );

	g_renderer = new QSGOpenGLRenderer();
	if (!g_renderer)
		return 1;

	g_controller = new LuaController(g_renderer);
	if (!g_controller)
		return 1;

	// start rendering
	m_active = TRUE;

	// create the GL view before showing the window
	GetClientRect(m_mainWnd, &rectClient);
	if (!CreateGLWindow(rectClient.right, rectClient.bottom))
		log_Log("SwitchToWindowed: failed to create GL view");

	// init lua and start client
	g_controller->execLua(c_apiFilename);
	g_controller->execLua(c_coreFilename);
	GetClientRect(m_glWnd, &rectClient);
	g_controller->resize(rectClient.right, rectClient.bottom);

	// show the main window
	ShowWindow(m_mainWnd, nCmdShow);
	UpdateWindow( m_mainWnd );

	g_lastTime = timeGetTime();

	// main message loop
	while( true )
	{
		if (g_useTimer) WaitMessage();
		else UpdateAndRender();

		// process all messages in the queue
		while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if( msg.message == WM_QUIT ) goto exit_main;
		//	if( !TranslateAccelerator(msg.hwnd, m_hAccelTable, &msg) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
	}
exit_main:

	delete g_controller; // manually tracked.
	g_controller = 0;
	g_renderer = 0; // free ref_ptr before main exits.

	log_Close();

	return 0;
}

/* ----------------------------------------------------------------------- */
/* Main window */

BOOL RegisterMainClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= 0;
	wcex.lpfnWndProc	= (WNDPROC)MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= c_mainWindowClass;
	wcex.hIconSm		= NULL;
	if( !RegisterClassEx(&wcex) ) return FALSE;
	return TRUE;
}

BOOL CreateMainWindow()
{
	m_mainWnd = CreateWindowEx( WS_EX_APPWINDOW, c_mainWindowClass, c_mainWindowTitle,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, 800, 600, NULL, NULL,
		m_hInstance, NULL  );
	if (!m_mainWnd) return FALSE;
	return TRUE;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_SIZE:
		ResizeGLWindow(LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN) {
			// ALT + ENTER: switch between Fullscreen and Windowed mode
			if( m_fullscreen ) SwitchToWindowed();
			else SwitchToFullscreen();
			return 0;
		}
		if (wParam == 76 /*L*/ && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
			ShowLogWindow();
			return 0;
		}
		break;
	case WM_ACTIVATE:
	case WM_SETFOCUS:
		if (m_glWnd)
			SetFocus(m_glWnd);
		break;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		PostQuitMessage(0);
		return 0;
	case WM_SYSCOMMAND:
		if (LOWORD(wParam) == IDM_OPEN_LOG)
		{
			ShowLogWindow();
		}
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/* ----------------------------------------------------------------------- */
/* Log window */

BOOL RegisterLogClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= 0;
	wcex.lpfnWndProc	= (WNDPROC)LogWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL; //LoadIcon(hInstance, (LPCTSTR)IDI_CLIENT);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(LTGRAY_BRUSH+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= c_logWindowClass;
	wcex.hIconSm		= NULL; //LoadIcon(hInstance, (LPCTSTR)IDI_CLIENT);
	if( !RegisterClassEx(&wcex) ) return FALSE;
	return TRUE;
}

BOOL CreateLogWindow()
{
	m_logWnd = CreateWindow( c_logWindowClass, c_logWindowTitle, WS_OVERLAPPEDWINDOW |
		WS_CLIPCHILDREN, CW_USEDEFAULT, 0, 500, 500, NULL, NULL, m_hInstance, NULL  );
	if (!m_logWnd) return FALSE;
	return TRUE;
}

LRESULT CALLBACK LogWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_CREATE:
		m_logBox = CreateWindow( "EDIT", "", WS_CHILD | WS_VISIBLE | ES_LEFT |
			ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL |
			WS_HSCROLL, 0, 0, 0, 0, hWnd, NULL, m_hInstance, NULL );
		//SendMessage(hLogBox, WM_SETFONT, (WPARAM)fontNew, MAKELPARAM(false, 0));
		break;
	case WM_ACTIVATE:
	case WM_SETFOCUS:
		SetFocus(m_logBox);
		break;
	case WM_SIZE:
		MoveWindow(m_logBox, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		break;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		//PostQuitMessage(0);
		break;
	case WM_SHOWWINDOW:
		g_logVisible = (wParam != FALSE);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

void ShowLogWindow()
{
	if (!g_logVisible || m_fullscreen)
	{
		if( m_fullscreen ) SwitchToWindowed();
		ShowWindow( m_logWnd, SW_SHOW );
	}
	if (GetActiveWindow() != m_logWnd)
	{
		BringWindowToTop( m_logWnd );
	}
}

/* ----------------------------------------------------------------------- */
/* OpenGL view */

#include <gl/gl.h>
#include <gl/glu.h>

static HDC m_glDC = 0;
static HGLRC m_glRC = 0;

BOOL RenderGLView();
BOOL CreateGLContext();
void ReleaseGLContext();

BOOL RegisterGLClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_OWNDC | CS_HREDRAW | CS_VREDRAW; // must have OwnDC.
	wcex.lpfnWndProc	= (WNDPROC)GLWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= c_glWindowClass;
	wcex.hIconSm		= NULL;
	if( !RegisterClassEx(&wcex) ) return FALSE;
	return TRUE;
}

BOOL CreateGLWindow(int width, int height)
{
	if (m_glWnd) {
		log_Log("... GL window already exists");
		return FALSE;
	}

	log_Log("... creating GL view");

	m_glWnd = CreateWindowEx(WS_EX_NOPARENTNOTIFY, c_glWindowClass, "",
		WS_VISIBLE | WS_CHILD, 0, 0, width, height, m_mainWnd,
		(HMENU)(100), m_hInstance, 0);

	if (!m_glWnd) {
		log_Log("... failed to create GL window");
		return FALSE;
	}

	if (!CreateGLContext()) {
		log_Log("... failed to create GL context");
		return FALSE;
	}

	ShowWindow(m_glWnd, SW_SHOW);

	return TRUE;
}

void ResizeGLWindow(int width, int height)
{
	if (m_active && !m_fullscreen)
	{
		if (m_glWnd && width > 0 && height > 0)
		{
			log_Log("GL view resized");
			MoveWindow(m_glWnd, 0, 0, width, height, TRUE);
			g_controller->resize(width, height);
		}
	}
}

void SwitchToWindowed()
{
	int width, height;

	// stop rendering while we fiddle things
	m_active = FALSE;

	// hide the main window to repaint the desktop.
	// if changing mode, hide without repaint instead.
	ShowWindow(m_mainWnd, SW_HIDE);

	DestroyGLWindow();

	// change window style
	SetWindowLong(m_mainWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN);
	SetWindowPos(m_mainWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
		SWP_NOZORDER | SWP_FRAMECHANGED);

	width = m_rectWindow.right - m_rectWindow.left;
	height = m_rectWindow.bottom - m_rectWindow.top;

	// restore window position
	SetWindowPos(m_mainWnd, HWND_NOTOPMOST, m_rectWindow.left, m_rectWindow.top,
				 width, height, 0 );

	// create a new GL view covering the client area
	if (!CreateGLWindow(width, height))
		log_Log("SwitchToWindowed: failed to create GL view");

	// start listening to WM_SIZE again
    m_fullscreen = FALSE;

	// allow rendering again
	m_active = TRUE;
	g_controller->resize(width, height);

	ShowWindow(m_mainWnd, SW_SHOW);
	UpdateWindow(m_mainWnd);
}

void SwitchToFullscreen()
{
	DEVMODE devMode;
	int width, height;

	// stop rendering while we fiddle things
	m_active = FALSE;

	// save window position
	GetWindowRect( m_mainWnd, &m_rectWindow );

	// hide the main window without redraw to avoid extra drawing.
	SetWindowPos(m_mainWnd, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOREDRAW |
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

	DestroyGLWindow();

	// start to ingore WM_SIZE before changing screen mode
    m_fullscreen = TRUE;

	// get current display settings
	devMode.dmSize = sizeof(DEVMODE);
	devMode.dmDriverExtra = 0;
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
	width = devMode.dmPelsWidth;
	height = devMode.dmPelsHeight;

	// change window style and move main window to cover screen
	SetWindowLong(m_mainWnd, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN);
	SetWindowPos(m_mainWnd, HWND_TOPMOST, 0, 0, width, height,
		SWP_FRAMECHANGED | SWP_NOREDRAW);

	// create a new GL view covering the main window
	if (!CreateGLWindow(width, height))
		log_Log("SwitchToFullscreen: failed to create GL view");

	// allow rendering again
	m_active = TRUE;
	g_controller->resize(width, height);

	// show the main window to start the paint cycle
	ShowWindow(m_mainWnd, SW_SHOW);
	UpdateWindow(m_glWnd);
}

void DestroyGLWindow()
{
	if (m_glWnd)
	{
		DestroyWindow(m_glWnd);
		m_glWnd = 0;
		m_glDC = 0;
	}
}

LRESULT CALLBACK GLWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		m_glDC = GetDC(hWnd); // we have OwnDC.
		break; // default

	case WM_DESTROY:
		if (g_useTimer) KillTimer(hWnd, 1);
		ReleaseGLContext();
		break; // default

	case WM_ERASEBKGND:
		return TRUE;

	/*
	case WM_SETCURSOR:
		SetCursor(NULL); // hide the cursor?
		return TRUE;
	*/

	case WM_PAINT:
		if (m_active && RenderGLView())
		{
			ValidateRect(hWnd, NULL);
			if (g_useTimer) SetTimer(hWnd, 1, 1, NULL); // 16 = ~60 fps
			return 0L;
		}
		else
		{
			if (g_useTimer) KillTimer(hWnd, 1); // stop timer until WM_PAINT.
		}
		break; // default

	case WM_TIMER:
		if (g_useTimer) UpdateAndRender();
		return 0L;

	case WM_MOUSEMOVE:
		{
			int x = (short)LOWORD(lParam);
			int y = (short)HIWORD(lParam);
			g_controller->mouseMove(x, y);
		}
		return 0L;

	case WM_LBUTTONDOWN:
		g_controller->mouseButton(1, 1);
		return 0L;

	case WM_LBUTTONUP:
		g_controller->mouseButton(1, 0);
		return 0L;

	case WM_RBUTTONDOWN:
		g_controller->mouseButton(2, 1);
		return 0L;

	case WM_RBUTTONUP:
		g_controller->mouseButton(2, 0);
		return 0L;

	case WM_MBUTTONDOWN:
		g_controller->mouseButton(3, 1);
		return 0L;

	case WM_MBUTTONUP:
		g_controller->mouseButton(3, 0);
		return 0L;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (wParam == 'L' &&
			(GetAsyncKeyState(VK_MENU) & 0x8000) &&
			(GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
			ShowLogWindow();
			return 0;
		}
		if (!(lParam & 0x40000000)) // first key down event
		{
			int key = (int)wParam;
			g_controller->keyPress(key, 1);
		}
		if (message == WM_KEYDOWN) return 0L;
		if (wParam == VK_RETURN) {
			// ALT + ENTER: switch between Fullscreen and Windowed mode
			if( m_fullscreen ) SwitchToWindowed();
			else SwitchToFullscreen();
			return 0;
		}
		break; // WM_SYSKEYDOWN

	case WM_KEYUP:
	case WM_SYSKEYUP:
		{
			int key = (int)wParam;
			g_controller->keyPress(key, 0);
		}
		if (message == WM_KEYUP) return 0L;
		break; // WM_SYSKEYUP

	case WM_CHAR:
		{
			char buf = (char)wParam; // TODO
			g_controller->keyChars(&buf, 1);
		}
		return 0L;

	default:
		break; // default
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void UpdateAndRender()
{
	if (m_active)
	{
		DWORD now = timeGetTime();
		DWORD delta = now - g_lastTime;
		//if (delta > 0)
		{
			g_lastTime = now;
			g_controller->update(delta);

			if (RenderGLView())
			{
				ValidateRect(m_glWnd, NULL);
			}
		}
	}
}

BOOL RenderGLView()
{
	RECT rect;

	if (!m_glRC)
	{
		log_Log("RenderGLView: no rendering context");
		return FALSE;
	}

	if (!IsWindowVisible(m_glWnd) || IsIconic(m_glWnd))
	{
		//log_Log("RenderGLView: window not visible");
		return FALSE;
	}

	if (GetClipBox(m_glDC, &rect) == NULLREGION)
	{
		//log_Log("RenderGLView: window is obscured");
		return FALSE;
	}

	g_controller->render();

	// commit all drawing commands.
	glFinish();

	// swap the window buffers.
	if (!SwapBuffers( m_glDC ))
	{
		log_Log("RenderGLView: failed to swap buffers");
		return FALSE;
	}

	return TRUE;
}

int GetDisplayDepth()
{
	DEVMODE devMode;
	devMode.dmSize = sizeof(DEVMODE);
	devMode.dmDriverExtra = 0;
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
	return (int) devMode.dmBitsPerPel;
}

BOOL CreateGLContext()
{
	PIXELFORMATDESCRIPTOR pfd;
	int nPixelFormat;

	// only permitted to do this once per window handle.
	memset( &pfd, 0, sizeof(pfd) );
	pfd.nSize      = sizeof(pfd);
	pfd.nVersion   = 1;
	pfd.dwFlags    = PFD_DOUBLEBUFFER |
					 PFD_SUPPORT_OPENGL |
					 PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = GetDisplayDepth(); // match display.
	//pfd.cDepthBits = 16; // at least 16 bits.
	pfd.iLayerType = PFD_MAIN_PLANE;

	nPixelFormat = ChoosePixelFormat( m_glDC, &pfd );
	if( nPixelFormat == 0 ) {
		log_Log("no matching pixel format");
		return FALSE;
	}

	memset( &pfd, 0, sizeof(pfd) );
	if (!DescribePixelFormat( m_glDC, nPixelFormat, sizeof(pfd), &pfd )) {
		log_Log("cannot describe pixel format");
		return FALSE;
	}

	if (!SetPixelFormat( m_glDC, nPixelFormat, &pfd )) {
		log_Log("cannot set pixel format");
		return FALSE;
	}

	// create the rendering context.
	m_glRC = wglCreateContext( m_glDC );
	if( !m_glRC ) {
		log_Log("cannot create rendering context");
		return FALSE;
	}

	// make the rendering context current.
	if (!wglMakeCurrent( m_glDC, m_glRC )) {
		log_Log("cannot make rendering context current");
		return FALSE;
	}

	g_renderer->initialise();

	return TRUE;
}

void ReleaseGLContext()
{
	g_renderer->shutdown();

	wglMakeCurrent( NULL, NULL );

	if (m_glRC)
		wglDeleteContext( m_glRC );

	m_glRC = NULL;
}
