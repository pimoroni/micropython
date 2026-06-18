# Simulator build & test tools

Helpers for the badgeware webassembly MicroPython port.

## build.sh

Builds the port (defaults to `VARIANT=pyscript`) and deploys
`micropython.mjs` / `micropython.wasm` into `web/badgeware-web/simulator/`.

```sh
tools/build.sh              # build + deploy
tools/build.sh --clean      # make clean, then build + deploy
tools/build.sh --no-deploy  # build only
VARIANT=standard tools/build.sh
```

It activates emscripten from `web/emsdk/emsdk_env.sh` if `emcc` isn't already on
`PATH`.

> If you change C module registrations (`MP_REGISTER_MODULE`) or the frozen
> manifest and hit a stale `genhdr`/`frozen_content` error, run with `--clean`.

## test.mjs

Headless smoke tests, run under Node against a built `micropython.mjs`. It mocks
`WorkerGlobalScope.worker` (as the real worker does) so the `EM_ASM` bridges in
`simulator/machine.cpp` and `simulator/st7789.c` have something to talk to.

```sh
node tools/test.mjs                                  # uses ../build-pyscript/micropython.mjs
node tools/test.mjs path/to/micropython.mjs
```

Covers: `machine` (Pin/PWM/ADC/RTC + caselight bridge), `st7789` (drop-in API +
framebuffer buffer protocol), the cooperative yield (input received inside a
tight loop), pause (`worker.running && worker.paused` halts execution), and that
the networking modules from the manifest are frozen in.

Exits non-zero if any check fails.
