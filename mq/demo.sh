#!/bin/sh

/usr/libexec/flua demo_receiver.lua &

/usr/libexec/flua demo_sender.lua <<EOF
HELLO WORLD
EOF

ps ax | /usr/libexec/flua demo_sender.lua

/usr/libexec/flua demo_kill.lua

wait %1
