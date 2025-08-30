
SHLIB_NAME=	sendfile.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_sendfile.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	sendfile.3lua

.include <bsd.lib.mk>
