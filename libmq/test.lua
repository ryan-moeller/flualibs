local mq = require'mq'
local q = mq.open("/myqueue", mq.O_CREAT + mq.O_WRONLY, 438)
