# test that frozen bytecode reports local variable names to sys.settrace

import sys

try:
    sys.settrace
    import frzlocals
except (AttributeError, ImportError):
    print("SKIP")
    raise SystemExit


def trace(frame, event, arg):
    if event == "return" and frame.f_code.co_name in ("scroll", "update", "plain"):
        print(frame.f_code.co_name, sorted(frame.f_locals))
    return trace


sys.settrace(trace)
update = frzlocals.scroll(3)
update()
frzlocals.plain(1, 2)
sys.settrace(None)
