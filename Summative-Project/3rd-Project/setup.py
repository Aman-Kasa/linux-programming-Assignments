from setuptools import Extension, setup

# Define the C Extension module configuration
sensor_analysis_module = Extension(
    "sensor_analysis",
    sources=["sensor_analysis.c"],
    extra_compile_args=["-O3", "-std=c99"],  # Enable compiler optimizations
)

setup(
    name="sensor_analysis",
    version="1.0.0",
    description="High-performance C extension module for smart agriculture sensor data analysis.",
    author="ALU Smart Agriculture Platform",
    ext_modules=[sensor_analysis_module],
)
