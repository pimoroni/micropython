# test importing a .mpy file that has a local variable names section
# requires: slice

try:
    import sys, io, vfs

    sys.implementation._mpy
    io.IOBase
except (ImportError, AttributeError):
    print("SKIP")
    raise SystemExit

if sys.implementation._mpy & 0xFF != 6:
    print("SKIP")
    raise SystemExit


class UserFile(io.IOBase):
    def __init__(self, data):
        self.data = memoryview(data)
        self.pos = 0

    def readinto(self, buf):
        n = min(len(buf), len(self.data) - self.pos)
        buf[:n] = self.data[self.pos : self.pos + n]
        self.pos += n
        return n

    def ioctl(self, req, arg):
        if req == 4:  # MP_STREAM_CLOSE
            return 0
        return -1


class UserFS:
    def __init__(self, files):
        self.files = files

    def mount(self, readonly, mksfs):
        pass

    def umount(self):
        pass

    def stat(self, path):
        if path in self.files:
            return (32768, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        raise OSError

    def open(self, path, mode):
        return UserFile(self.files[path])


# mod.py, "def f(a): b = a + 1; return b; print(f(1))", followed by the
# names section: tag "L", then names of <module> (none) and f ("a", "b")
user_files = {
    "/mod.mpy": b"M\x06\x00\x1f\x05\x00\x0cmod.py\x00\x0f\x02f\x00\x81w\x02a\x00\x81,\x10\x06\x01d@2\x00\x16\x02\x11\x03\x11\x02\x814\x014\x01YQc\x01`\x19\x08\x02\x04 $\xb0\x81\xf2\xc1\xb1cL\x00\x02\x02a\x00\x02b\x00",
}

# create and mount a user filesystem
vfs.mount(UserFS(user_files), "/userfs")
sys.path.append("/userfs")

# the loader stops after the raw code, so the names section is ignored
import mod

# unmount and undo path addition
vfs.umount("/userfs")
sys.path.pop()
