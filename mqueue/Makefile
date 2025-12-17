SHLIB_NAME=	mqueue.so
SRCS+=	lua_mqueue.c
CFLAGS+=${FLUA_CFLAGS}
LDADD+=	-lrt
MAN=	mqueue.3lua

# XXX: not perfect, but works for the CI
.if !exists(/usr/share/man/man4/mqueuefs.4.gz)
mantransform: .PHONY
	sed -i '' -e 's/mqueuefs 4/mqueuefs 5/' ${MAN}
all-man: mantransform
manlint: mantransform
.endif

.include "../Makefile.inc"
.include <bsd.lib.mk>
