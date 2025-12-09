SHLIB_NAME=	nv.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_nv.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-lnv

MAN=	nv.3lua

.include <bsd.lib.mk>
