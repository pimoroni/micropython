# Shared frozen manifest for the badgeware simulator variants.
#
# Pulls in the base port manifest (asyncio), the Python standard-library modules
# badgeware and its apps rely on, the fetch-backed networking shims, and the
# common firmware modules. Board-specific variants include this from their own
# manifest.py.

include("$(PORT_DIR)/variants/manifest.py")

# Python standard library (from micropython-lib).
require("abc")
require("base64")
require("collections")
require("collections-defaultdict")
require("copy")
require("datetime")
require("fnmatch")
require("functools")
require("gzip")
require("hmac")
require("html")
require("inspect")
require("io")
require("itertools")
require("locale")
require("logging")
require("operator")
require("os")
require("os-path")
require("pathlib")
require("stat")
require("tarfile")
require("tarfile-write")
require("time")
require("unittest")
require("uu")
require("zlib")

# Networking. A browser can't open raw sockets, so the socket-based `requests`
# and `urllib.urequest` from micropython-lib can't run here. Instead we freeze
# fetch-backed shims (simulator/modules/, backed by the _jsfetch C module) under
# the usual module names. `umqtt.simple` is raw-TCP and has no fetch equivalent,
# so it's frozen for source compatibility but is non-functional in the simulator.
module("requests.py", base_path="$(PORT_DIR)/simulator/modules", opt=3)
package("urllib", ("urequest.py",), base_path="$(PORT_DIR)/simulator/modules", opt=3)
require("umqtt.simple")

# AsyncFetch: a cooperative, streaming HTTP client, shimmed onto the Fetch API.
module("fetch.py", base_path="$(PORT_DIR)/simulator/modules", opt=3)

# Common firmware modules brought in from tufty2350/modules/common. easing,
# pimoroni and board are unchanged; wifi is a simulator shim (no real WLAN —
# it just reports connected, since the browser network is reachable via fetch).
module("easing.py", base_path="$(PORT_DIR)/simulator/modules", opt=3)
module("pimoroni.py", base_path="$(PORT_DIR)/simulator/modules", opt=3)
module("board.py", base_path="$(PORT_DIR)/simulator/modules", opt=3)
module("wifi.py", base_path="$(PORT_DIR)/simulator/modules", opt=3)
# secrets loader (unchanged from Tufty): reads /secrets, falling back to the
# bundled /system/secrets.py default.
module("secrets.py", base_path="$(PORT_DIR)/simulator/modules", opt=3)
# ntptime shim: no UDP/socket; reads the host clock instead.
module("ntptime.py", base_path="$(PORT_DIR)/simulator/modules", opt=3)
