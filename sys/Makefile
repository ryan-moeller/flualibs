DIRS=	FLUA_SYS
FLUA_SYS=	${LIBDIR}/flua/sys

.include <bsd.dirs.mk>

SUBDIR=	\
	acl \
	capsicum \
	cpuset \
	event \
	eventfd \
	extattr \
	limits \
	linker \
	mac \
	module \
	reboot \
	socket \
	stat \
	sysctl \
	uio \

.if exists(/usr/include/sys/inotify.h)
SUBDIR+=inotify
.endif

.include <bsd.subdir.mk>
