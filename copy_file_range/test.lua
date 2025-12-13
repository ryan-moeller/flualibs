-- Expects stdin and stdout to be files, e.g. flua test.lua </COPYRIGHT >/tmp/f
local copy_file_range = require('copy_file_range')

assert(copy_file_range(io.stdin, nil, io.stdout, nil, copy_file_range.SSIZE_MAX, 0))
