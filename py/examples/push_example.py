from __future__ import absolute_import
from __future__ import print_function
import logging
from six.moves import range
logging.basicConfig(level=logging.DEBUG)

import gevent

from toku.client import TokuClient

client = TokuClient(('localhost', 4001))
for i in range(100):
    client.send_push('hello world %i' % i)
    gevent.sleep(0)

print(client.send_request('oh hi'))

for i in range(100):
    client.send_push('hello world %i' % i)
    gevent.sleep(0)

print(client.send_request('oh hi'))
