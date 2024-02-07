
SHLIB_NAME=	fileno.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_fileno.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	fileno.3lua

.include <bsd.lib.mk>
