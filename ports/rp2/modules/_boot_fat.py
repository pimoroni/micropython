import vfs
import machine, rp2

# 1441792
bdev_root = rp2.Flash(start=0, len=4096 * 100)
bdev_storage = rp2.Flash(start=4096 * 100, len=4096 * 250)

try:
    vfs_root = vfs.VfsFat(bdev_root)
    vfs.mount(vfs_root, "/", readonly=True)
except:
    vfs.VfsFat.mkfs(bdev_root)
    vfs_root = vfs.VfsFat(bdev_root)
    vfs_root.label("Root")
    vfs.mount(vfs_root, "/", readonly=True)

try:
    vfs_storage = vfs.VfsFat(bdev_storage)
    vfs.mount(vfs_storage, "/storage")
except:
    vfs.VfsFat.mkfs(bdev_storage)
    vfs_storage = vfs.VfsFat(bdev_storage)
    vfs_storage.label("Storage")
    vfs.mount(vfs_storage, "/storage")

del bdev_root, bdev_storage, vfs_storage, vfs_root
