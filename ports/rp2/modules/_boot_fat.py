import vfs
import machine, rp2


# Try to mount the filesystem, and format the flash if it doesn't exist.
bdev = rp2.Flash()
try:
    fat = vfs.VfsFat(bdev)
    fat.label("Badger2350")
    vfs.mount(fat, "/")
except:
    vfs.VfsFat.mkfs(bdev)
    fat = vfs.VfsFat(bdev)
    fat.label("Badger2350")
    vfs.mount(fat, "/")

del vfs, bdev