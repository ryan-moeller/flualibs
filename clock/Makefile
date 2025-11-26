SHLIB_NAME=	clock.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_clock.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	clock.3lua

.include <bsd.lib.mk>
