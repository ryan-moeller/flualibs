SRCTOP?= /usr/src
.export SRCTOP

SUBDIR= \
	fileno \
	libbe \
	libfetch \
	libkldstat \
	libkqueue \
	libmd \
	libmq \
	libroken \
	libxor \

.include <bsd.subdir.mk>
