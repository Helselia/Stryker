import os
from setuptools import setup
from setuptools.extension import Extension
import glob

try:
    from Cython.Build import cythonize
    ext = 'pyx'
except ImportError:
    cythonize = None
    ext = 'c'


extensions = []
for file in glob.glob('py/toku/*.%s' % ext):
    package = os.path.splitext(os.path.basename(file))[0]
    extensions.append(Extension(
        'toku.%s' % package,
        [file],
        extra_compile_args=['-O3', '-g']
    ))

if cythonize:
    extensions = cythonize(extensions, gdb_debug=True)

setup(
    name='toku',
    version='0.2.20',
    author='Constanze',
    author_email='cstanze@helselia.dev',
    url="http://github.com/Helselia/Toku",
    description='A really simple stream based RPC - with a gevent client/server implementation',
    license='MIT',
    package_dir={
        '': 'py'
    },
    packages=['toku'],
    ext_modules=extensions,
    tests_require=['pytest'],
    setup_requires=['pytest-runner']
)
