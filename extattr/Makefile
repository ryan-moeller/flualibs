SHLIB_NAME=	extattr.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_extattr.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	extattr.3lua

.include <bsd.lib.mk>
