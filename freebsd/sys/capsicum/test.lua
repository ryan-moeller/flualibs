local cap = require('freebsd.sys.capsicum')

cap.enter()
assert(cap.sandboxed())
