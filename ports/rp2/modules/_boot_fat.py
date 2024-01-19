import os
import machine, rp2

# 1441792
bdev_root = rp2.Flash(start=0, len=4096 * 100)
bdev_storage = rp2.Flash(start=4096 * 100, len=4096 * 250)

try:
    vfs_root = os.VfsFat(bdev_root)
    os.mount(vfs_root, "/", readonly=True)
except:
    os.VfsFat.mkfs(bdev_root)
    vfs_root = os.VfsFat(bdev_root)
    vfs_root.label("Root")
    os.mount(vfs_root, "/", readonly=True)

try:
    vfs_storage = os.VfsFat(bdev_storage)
    os.mount(vfs_storage, "/storage")
except:
    os.VfsFat.mkfs(bdev_storage)
    vfs_storage = os.VfsFat(bdev_storage)
    vfs_storage.label("Storage")
    os.mount(vfs_storage, "/storage")

del os, bdev_root, bdev_storage, vfs_storage, vfs_root