package = "Copas"
version = "cvs-2"
source = {
   url = "cvs://:pserver:anonymous:@cvs.luaforge.net:/cvsroot/copas",
   cvs_tag = "HEAD",
}
description = {
   summary = "Coroutine Oriented Portable Asynchronous Services",
   detailed = [[
      Copas is a dispatcher based on coroutines that can be used by
      TCP/IP servers. It uses LuaSocket as the interface with the
      TCP/IP stack. A server registered with Copas should provide a
      handler for requests and use Copas socket functions to send
      the response. Copas loops through requests and invokes the
      corresponding handlers. For a full implementation of a Copas
      HTTP server you can refer to Xavante as an example. 
   ]],
   license = "MIT/X11",
   homepage = "http://www.keplerproject.org/copas/"
}
dependencies = {
   "lua >= 5.1",
   "luasocket >= 2.0",
   "coxpcall >= 1.13",
}
build = {
   type = "module",
   modules = { copas = "src/copas/copas.lua" }
}
