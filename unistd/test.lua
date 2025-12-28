-- Expects stdin and stdout to be files, e.g. flua test.lua </COPYRIGHT >/tmp/f
local limits = require('sys.limits')
local unistd = require('unistd')
local copy_file_range = unistd.copy_file_range

assert(copy_file_range(io.stdin, nil, io.stdout, nil, limits.SSIZE_MAX, 0))
