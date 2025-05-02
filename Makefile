SRCTOP?= /usr/src
.export SRCTOP

SUBDIR= \
	fileno \
	libbe \
	libbsddialog \
	libfetch \
	libkldstat \
	libkqueue \
	libmd \
	libmq \
	libroken \
	libxor \

.include <bsd.subdir.mk>
