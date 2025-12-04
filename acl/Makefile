SHLIB_NAME=	acl.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_acl.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	acl.3lua

.include <bsd.lib.mk>
