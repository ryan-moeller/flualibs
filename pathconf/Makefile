SHLIB_NAME=	pathconf.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_pathconf.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	pathconf.3lua

.include <bsd.lib.mk>
