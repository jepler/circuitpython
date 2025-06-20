import sys

try:
    sys.settrace
except AttributeError:
    print("SKIP")
    raise SystemExit

if sys.version.startswith("3.12"):
    # There is a CPython change in settrace that is reverted in 3.13!
    print("WARNING: this test will fail when compared to CPython 3.12.x behaviour")


def trace_tick_handler(frame, event, arg):
    print("FRAME", frame)
    return None


def f():
    x = 3
    return x


sys.settrace(trace_tick_handler)
f()
sys.settrace(None)
