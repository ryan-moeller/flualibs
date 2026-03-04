SUBDIR= \
	aio \
	b64 \
	capsicum_helpers \
	dirent \
	fcntl \
	grp \
	libcasper \
	libfetch \
	libifconfig \
	libkvm \
	libmagic \
	libmd \
	libnv \
	libpathconv \
	libpthread \
	libutil \
	mqueue \
	netdb \
	netinet \
	pwd \
	readpassphrase \
	signal \
	stdio \
	sys \
	syslog \
	time \
	unistd \
	xor \

.include "Makefile.inc"

.if exists(${SRCTOP}/contrib/bsddialog/lib/bsddialog.h)
SUBDIR+=libbsddialog
.endif
.if exists(/usr/include/libnvpair.h)
SUBDIR+=libbe
.endif
.if exists(/usr/include/dpv.h)
SUBDIR+=libdpv
.endif

MANDOC_CMD=	sh ../manlint.sh

.include <bsd.subdir.mk>
