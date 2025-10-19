SHLIB_NAME=	magic.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_magic.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-lmagic

MAN=	magic.3lua

.include <bsd.lib.mk>
