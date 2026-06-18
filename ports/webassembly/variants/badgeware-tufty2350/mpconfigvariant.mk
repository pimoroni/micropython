# Badgeware simulator variant for the Pimoroni Tufty 2350.

# Shared badgeware build (picovector + simulator modules).
include variants/badgeware.mk

# Tufty 2350 picovector build flag.
CFLAGS += -DTUFTY=1

JSFLAGS += -s ALLOW_MEMORY_GROWTH

FROZEN_MANIFEST ?= variants/badgeware-tufty2350/manifest.py
