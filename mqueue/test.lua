local mqueue = require('mqueue')

local q = mqueue.open("/myqueue", mqueue.O_CREAT + mqueue.O_WRONLY, 438)
