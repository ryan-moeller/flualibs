SHLIB_NAME=	kvm.so
SRCS+=	lua_kvm.c
CFLAGS+=${FLUA_CFLAGS}
LDADD+=	-lkvm
MAN=	kvm.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
