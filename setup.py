import os
from setuptools import setup
from setuptools.extension import Extension
import glob

try:
  from Cython.Build import cythonize
  ext = "pyx"
except ImportError:
  cythonize = None
  ext = "c"

exts = []
for file in glob.glob("py/toku/*.%s" % ext):
  package = os.path.splitext(os.path.basename(file))[0]
  exts.append(Extension(
    "toku.%s" % package,
    [file],
    extra_compile_args=["-O3"]
  ))

if cythonize:
  exts = cythonize(exts)

setup(
  name='toku',
  version='0.2.13',
  author='Constanze',
  author_email='cstanze@helselia.dev',
  url='https://github.com/Helselia/Toku',
  description='A simple stream based RPC - with a gevent client/server implementation. A fork of discord/loqui',
  license='MIT',
  package_dir={
    '': 'py'
  },
  packages=['toku'],
  ext_modules=exts
)
