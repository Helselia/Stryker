from __future__ import absolute_import
from .client_bench_base import run_client_bench
from gevent.monkey import patch_all
patch_all()

from toku.client import TokuClient

client = TokuClient(('localhost', 8080))
run_client_bench(client, concurrency=100)
