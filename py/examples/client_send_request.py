from __future__ import absolute_import
from __future__ import print_function
from toku.client import TokuClient

client = TokuClient(('localhost', 4001))
print(len(client.send_request('hello world')))