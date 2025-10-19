SHLIB_NAME=	md.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_md.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-lmd

MAN=	md.3lua

.include <bsd.lib.mk>
