import vfs
import machine, rp2


# Try to mount the filesystem, and format the flash if it doesn't exist.
bdev = rp2.Flash()
try:
    vfs.mount(vfs.VfsFat(bdev), "/")
except:
    vfs.VfsFat.mkfs(bdev)
    msc = vfs.VfsFat(bdev)
    msc.label("RP2_MSC")
    vfs.mount(msc, "/")

del vfs, bdev
