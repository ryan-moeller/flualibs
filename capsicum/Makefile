SHLIB_NAME=	capsicum.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_capsicum.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	capsicum.3lua

.include <bsd.lib.mk>
