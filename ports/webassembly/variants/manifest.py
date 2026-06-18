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
# fetch-backed shims (simulator/net/, backed by the _jsfetch C module) under the
# usual module names. `umqtt.simple` is raw-TCP and has no fetch equivalent, so
# it's frozen for source compatibility but is non-functional in the simulator.
module("requests.py", base_path="$(PORT_DIR)/simulator/net", opt=3)
package("urllib", ("urequest.py",), base_path="$(PORT_DIR)/simulator/net", opt=3)
require("umqtt.simple")

# AsyncFetch: a cooperative, streaming HTTP client, shimmed onto the Fetch API.
module("fetch.py", base_path="$(PORT_DIR)/modules", opt=3)
