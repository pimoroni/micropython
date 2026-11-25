# test that the settrace exception event says whether the frame handles it

import sys

try:
    sys.settrace
except AttributeError:
    print("SKIP")
    raise SystemExit


def inner():
    raise ValueError("boom")


def caught():
    try:
        inner()
    except ValueError:
        pass


def leaks():
    inner()


def trace(frame, event, arg):
    if event == "exception":
        print(frame.f_code.co_name, arg[0].__name__, arg[1], arg[2])
    return trace


sys.settrace(trace)
caught()
try:
    leaks()
except ValueError:
    pass
sys.settrace(None)
