DIRS+=	FLUA_CASPER
FLUA_CASPER=	${LIBDIR}/flua/casper

.include <bsd.dirs.mk>

.if exists(/usr/include/casper/cap_netdb.h)
SUBDIR+=netdb
.endif

.include <bsd.subdir.mk>
