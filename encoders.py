from collections import OrderedDict

ENCODERS = OrderedDict()

try:
  import json

  ENCODERS["json"] = json
except ImportError:
  pass

try:
  import erlpack

  class _erl:
    dumps = erlpack.pack
    loads = erlpack.unpack
  
  ENCODERS["erlpack"] = _erl
except ImportError:
  pass

try:
  import msgpack

  ENCODERS["msgpack"] = msgpack
except ImportError:
  pass
