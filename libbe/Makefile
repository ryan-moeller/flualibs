SHLIB_NAME=	be.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_be.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-lbe

MAN=	be.3lua

.include <bsd.lib.mk>
