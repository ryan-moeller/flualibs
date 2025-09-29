#!/bin/sh

# Start a small test server using socat and ryan-moeller/lua-httpd

: ${TMPDIR:="/tmp"}
export TMPDIR

if [ ! -d "${TMPDIR}" ]; then
	echo "TMPDIR=${TMPDIR} does not exist!"
	exit 1
fi

basedir=$(realpath "$(dirname "$0")")
port=${1:-8080}

httpd_lua="${TMPDIR}/httpd.lua"
#httpd_lua=~ryan/lua-httpd/httpd.lua # if you're me

testdir=$(mktemp -d -t fetchtest)

cleanup() {
	rm -rf "${testdir}"
}

trap cleanup INT TERM EXIT

test -f "${httpd_lua}" || fetch -o "${httpd_lua}" \
    https://raw.githubusercontent.com/ryan-moeller/lua-httpd/main/httpd.lua
ln -s "${httpd_lua}" "${testdir}"
ln -s "${basedir}/test-server.lua" "${testdir}"

cat <<EOF
TMPDIR:  ${TMPDIR}
basedir: ${basedir}
testdir: ${testdir}
port:    ${port}
EOF

# pkg install socat
socat \
    TCP-LISTEN:${port},bind=localhost,reuseaddr,fork \
    EXEC:"/usr/libexec/flua test-server.lua",chdir="'${testdir}'"
