import vfs
import machine, rp2

blocks_root = 100
blocks_total = rp2.Flash().ioctl(4, None)
block_size = rp2.Flash().ioctl(5, None)
blocks_storage = blocks_total - blocks_root
bdev_root = rp2.Flash(start=0, len=block_size * blocks_root)
bdev_storage = rp2.Flash(start=block_size * blocks_root, len=block_size * blocks_storage)

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
