include("$(PORT_DIR)/boards/manifest.py")

require("bundle-networking")

# Bluetooth
require("aioble", client=True, central=True, l2cap=True, security=True)
