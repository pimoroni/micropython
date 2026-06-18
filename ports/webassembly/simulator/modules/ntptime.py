# Simulator shim for ntptime.
#
# NTP is raw UDP, which a browser can't speak. But the host already knows the
# correct wall-clock time, so time() reads the system clock and settime() writes
# it to the RTC, mirroring the real ntptime API (host / timeout / time /
# settime) without touching a socket.
from time import gmtime, time as _time

# Kept for API compatibility; ignored by the shim.
host = "pool.ntp.org"
timeout = 1


def time():
    return int(_time())


# The RTC is set in UTC time, matching the firmware ntptime.
def settime():
    t = time()
    import machine

    tm = gmtime(t)
    machine.RTC().datetime((tm[0], tm[1], tm[2], tm[6] + 1, tm[3], tm[4], tm[5], 0))
