# Performance benchmarks for CircuitPython using pytest-codspeed.
#
# These benchmarks wrap the existing perf_bench scripts, running the same
# algorithmic workloads with CodSpeed instrumentation to detect performance
# regressions in the benchmark implementations.
#
# The perf_bench scripts originate from MicroPython/pyperformance and exercise
# core interpreter operations: floating-point math, string handling, object
# allocation, generators, and classic algorithm benchmarks (FFT, N-Queens, etc.).

import importlib
import os
import sys

import pytest

# Add the perf_bench directory to the import path so benchmark modules can be loaded.
_PERF_BENCH_DIR = os.path.join(os.path.dirname(__file__), "..", "tests", "perf_bench")
sys.path.insert(0, os.path.abspath(_PERF_BENCH_DIR))

# Target parameters for PC-level workloads (same convention as run-perfbench.py).
_TARGET_N = 1000
_TARGET_M = 100


def _select_params(bm_params):
    """Select the best matching benchmark parameters for the target N, M.

    Uses the same selection logic as tests/perf_bench/benchrun.py:
    pick the largest (N, M) key where 10*N <= 12*TARGET_N and M <= TARGET_M.
    """
    cur_nm = (0, 0)
    param = None
    for nm, p in bm_params.items():
        if 10 * nm[0] <= 12 * _TARGET_N and nm[1] <= _TARGET_M and nm > cur_nm:
            cur_nm = nm
            param = p
    return param


# Benchmark modules compatible with CPython.
# Excluded: viper_* (MicroPython viper emitter), core_import_mpy_* (need .mpy files).
_BENCH_MODULES = [
    "bm_chaos",
    "bm_fannkuch",
    "bm_fft",
    "bm_float",
    "bm_hexiom",
    "bm_nqueens",
    "bm_pidigits",
    "bm_wordcount",
    "core_locals",
    "core_qstr",
    "core_str",
    "core_yield_from",
    "misc_aes",
    "misc_mandel",
    "misc_pystone",
    "misc_raytrace",
]


@pytest.mark.parametrize("module_name", _BENCH_MODULES)
def test_perf_bench(benchmark, module_name):
    mod = importlib.import_module(module_name)
    params = _select_params(mod.bm_params)
    assert params is not None, f"No suitable params for {module_name}"
    run, _result = mod.bm_setup(params)
    benchmark(run)
