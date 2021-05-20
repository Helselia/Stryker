from __future__ import absolute_import
from gevent.monkey import patch_all
patch_all()

from .client_bench_base import run_client_bench
from toku.client import TokuHTTPUpgradeClient
client = TokuHTTPUpgradeClient(('localhost', 8080))
run_client_bench(client, concurrency=100)
