local devstat = require('devstat')

print(devstat.getversion())
assert(devstat.checkversion())
