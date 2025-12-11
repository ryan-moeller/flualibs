SHLIB_NAME=	confstr.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_confstr.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	confstr.3lua

.include <bsd.lib.mk>
