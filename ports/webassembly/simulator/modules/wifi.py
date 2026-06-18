# Simulator shim for the badge `wifi` module.
#
# There's no real WLAN to manage in the browser, and the network is already
# reachable through the fetch-backed `requests` / `fetch` modules, so this shim
# just reports a successful, connected state. It mirrors the firmware wifi API
# (connect / tick / status / is_connected / ip / ...) so apps that gate on
# connectivity keep working — it simply never has to wait or fail.

_STATUS_IDLE = 0
_STATUS_CONNECTING = 1
_STATUS_CONNECTED = 2
_STATUS_GOT_IP = 3

_status_text = {
    0: "Idle",
    1: "Connecting",
    2: "Connected",
    3: "Got IP",
    -1: "Connection failed.",
    -2: "Access point not found.",
    -3: "Incorrect password.",
}

_connected = False


def get_status(index):
    return _status_text.get(index, "Unknown")


def tick():
    # Real wifi.tick() drives a connection state machine; here we're either
    # connected or not, immediately.
    return _connected


def connect(ssid=None, psk=None, timeout=60, retries=5):
    global _connected
    _connected = True
    return True


def disconnect():
    global _connected
    _connected = False


def status():
    if _connected:
        return _STATUS_GOT_IP, get_status(_STATUS_GOT_IP)
    return _STATUS_IDLE, get_status(_STATUS_IDLE)


def is_connected():
    return _connected


def ipv4():
    return "127.0.0.1" if _connected else None


def ipv6():
    return None


def ip():
    return ipv4()


def subnet():
    return "255.255.255.0" if _connected else None


def gateway():
    return "127.0.0.1" if _connected else None


def nameserver():
    return "127.0.0.1" if _connected else None
