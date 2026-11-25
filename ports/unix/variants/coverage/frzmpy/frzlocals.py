# Used by tests/misc/sys_settrace_localnames_frozen.py.


def scroll(speed):
    offset = 0

    def update():
        nonlocal offset
        timedelta = speed * 2
        offset += timedelta
        return offset

    return update


def plain(a, b):
    total = a + b
    return total
