from setuptools import setup, Extension

setup(
    name="my_ext",
    version="1.0",
    description="CPython high-performance calculation extension",
    ext_modules=[Extension("my_ext", ["my_ext.c"])],
)
