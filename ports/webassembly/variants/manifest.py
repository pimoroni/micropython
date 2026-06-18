# The asyncio package is built from the standard implementation but with the
# core scheduler replaced with a custom scheduler that uses the JavaScript
# runtime (with setTimeout an Promise's) to contrtol the scheduling.

package(
    "asyncio",
    (
        "event.py",
        "funcs.py",
        "lock.py",
    ),
    base_path="$(MPY_DIR)/extmod",
    opt=3,
)

package(
    "asyncio",
    (
        "__init__.py",
        "core.py",
    ),
    base_path="$(PORT_DIR)",
    opt=3,
)

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
