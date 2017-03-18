# makefile for building game client
# see ../INSTALL for installation instructions
# see ../Makefile for further customization

# == CHANGE THE SETTINGS BELOW TO SUIT YOUR ENVIRONMENT =======================

# Your platform. See PLATS for possible values.
PLAT= none

CC= gcc
CPP= g++
CFLAGS= -O2 -Wall $(MYCFLAGS)
CPPFLAGS= -O2 -Wall $(MYCFLAGS)
AR= ar rcu
RANLIB= ranlib
RM= rm -f
LIBS= -lm $(MYLIBS)

MYCFLAGS= -I../lua-5.1.3/src
MYLDFLAGS= -L../lua-5.1.3/src/
MYLIBS= -lGL -llua -lX11

# == END OF USER SETTINGS. NO NEED TO CHANGE ANYTHING BELOW THIS LINE =========

PLATS= generic linux macosx mingw

CLIENT_O=	stb_image.o xlua.o XWinMain.o Logger.o LuaController.o \
	QSGNode.o QSGTransformNode.o QSGFrame.o QSGText.o QSGClipView.o \
	QSGViewport.o QSGTransform.o QSGOpenGLRenderer.o \
	QSGResource.o QSGTexture.o

CLIENT_T=	client

ALL_O= $(CLIENT_O)
ALL_T= $(CLIENT_T)

default: $(PLAT)

all:	$(ALL_T)

o:	$(ALL_O)

$(CLIENT_T): $(CLIENT_O)
	$(CPP) -o $@ $(MYLDFLAGS) $(CLIENT_O) $(LIBS)

clean:
	$(RM) $(ALL_T) $(ALL_O)

depend:
	@$(CC) $(CFLAGS) -MM *.c
	@$(CPP) $(CFLAGS) -MM *.cpp

echo:
	@echo "PLAT = $(PLAT)"
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "AR = $(AR)"
	@echo "RANLIB = $(RANLIB)"
	@echo "RM = $(RM)"
	@echo "MYCFLAGS = $(MYCFLAGS)"
	@echo "MYLDFLAGS = $(MYLDFLAGS)"
	@echo "MYLIBS = $(MYLIBS)"

# convenience targets for popular platforms

none:
	@echo "Please choose a platform:"
	@echo "   $(PLATS)"

generic:
	$(MAKE) all

linux:
	$(MAKE) all MYLIBS="-Wl,-E -ldl $(MYLIBS)"

macosx:
	$(MAKE) all

mingw:
	$(MAKE) "CLIENT_T=client.exe" \
	"AR=$(CC) -shared -o" "RANLIB=strip --strip-unneeded" \
	"MYCFLAGS=$(MYCFLAGS) -DWINDOWS" "MYLIBS=" \
	"MYLDFLAGS=$(MYLDFLAGS) -s" "CLIENT_O=$(CLIENT_O) win_main.o" client.exe

# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all $(PLATS) default o a clean depend echo none

# use "make depend >deps" and copy output here, excluding WinMain.o!
# DO NOT DELETE

stb_image.o: stb_image.c stb_image.h
stb_vorbis.o: stb_vorbis.c stb_vorbis.h
xlua.o: xlua.c xlua.h ../lua-5.1.3/src/lua.h ../lua-5.1.3/src/luaconf.h \
  ../lua-5.1.3/src/lauxlib.h ../lua-5.1.3/src/lua.h
Logger.o: Logger.cpp global.h Logger.h
LuaController.o: LuaController.cpp LuaController.h QSGObject.h \
  QSGRenderer.h QSGTransform.h QSGViewport.h QSGNode.h QSGTransformNode.h \
  QSGFrame.h QSGClipView.h QSGTexture.h QSGResource.h Logger.h \
  ../lua-5.1.3/src/lua.h ../lua-5.1.3/src/luaconf.h \
  ../lua-5.1.3/src/lauxlib.h ../lua-5.1.3/src/lua.h \
  ../lua-5.1.3/src/lualib.h xlua.h stb_image.h
QSGClipView.o: QSGClipView.cpp QSGClipView.h QSGTransformNode.h QSGNode.h \
  QSGObject.h QSGTransform.h QSGRenderer.h
QSGFrame.o: QSGFrame.cpp QSGFrame.h QSGTransformNode.h QSGNode.h \
  QSGObject.h QSGTransform.h QSGRenderer.h
QSGNode.o: QSGNode.cpp QSGNode.h QSGObject.h
QSGOpenGLRenderer.o: QSGOpenGLRenderer.cpp QSGOpenGLRenderer.h \
  QSGRenderer.h QSGObject.h QSGTransform.h QSGTexture.h QSGResource.h \
  QSGNode.h
QSGResource.o: QSGResource.cpp QSGResource.h QSGObject.h
QSGText.o: QSGText.cpp QSGText.h QSGTransformNode.h QSGNode.h QSGObject.h \
  QSGTransform.h
QSGTexture.o: QSGTexture.cpp QSGTexture.h QSGResource.h QSGObject.h
QSGTransform.o: QSGTransform.cpp QSGTransform.h
QSGTransformNode.o: QSGTransformNode.cpp QSGTransformNode.h QSGNode.h \
  QSGObject.h QSGTransform.h QSGRenderer.h
QSGViewport.o: QSGViewport.cpp QSGViewport.h QSGNode.h QSGObject.h \
  QSGTransform.h QSGRenderer.h
XWinMain.o: XWinMain.cpp global.h Logger.h LuaController.h QSGObject.h \
  QSGOpenGLRenderer.h QSGRenderer.h QSGTransform.h

# (end of Makefile)
