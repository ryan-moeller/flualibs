-- Expects stdin and stdout to be files, e.g. flua test.lua </COPYRIGHT >/tmp/f
local unistd = require('unistd')
local copy_file_range = unistd.copy_file_range

assert(copy_file_range(io.stdin, nil, io.stdout, nil, unistd.SSIZE_MAX, 0))
