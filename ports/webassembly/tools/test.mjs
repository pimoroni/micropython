// Smoke tests for the badgeware webassembly MicroPython build, exercising the
// simulator-specific modules without a browser:
//
//   - machine   (Pin / PWM / ADC / RTC and the worker.machine bridge)
//   - st7789    (drop-in display driver + framebuffer buffer protocol)
//   - cooperative yield (input received mid tight-loop)
//   - pause     (worker.running && worker.paused halts execution)
//   - frozen networking modules from the manifest
//
// Usage:
//   node tools/test.mjs [path/to/micropython.mjs]
//
// Defaults to ../build-pyscript/micropython.mjs relative to this script. It
// stands in a mock `WorkerGlobalScope.worker` so the EM_ASM bridges in
// machine.cpp / st7789.c have something to talk to, exactly as the real worker
// (badgeware-web/simulator/micropython.worker.js) provides.

import { fileURLToPath, pathToFileURL } from "node:url";
import { dirname, resolve } from "node:path";

const __dirname = dirname(fileURLToPath(import.meta.url));
const mjsPath = process.argv[2]
  ? resolve(process.argv[2])
  : resolve(__dirname, "../build-pyscript/micropython.mjs");

let pass = 0;
let fail = 0;
let skipped = 0;
const ok = (name, cond, detail = "") => {
  if (cond) { pass++; console.log(`  ok   ${name}`); }
  else { fail++; console.log(`  FAIL ${name}${detail ? "  — " + detail : ""}`); }
};
const skip = (name, detail = "") => {
  skipped++; console.log(`  skip ${name}${detail ? "  — " + detail : ""}`);
};
const group = (name) => console.log(`\n# ${name}`);
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

// Mock worker global, mirroring micropython.worker.js.
let stdoutCount = 0;
globalThis.WorkerGlobalScope = { worker: {
  running: false,
  paused: false,
  input: 0,
  flip_hires: () => {},
  flip_lores: () => {},
  machine: { gpio: {}, gpio_in: {}, pwm: {}, caselights: [0, 0, 0, 0], adc: {} },
  update_caselights: () => { globalThis.WorkerGlobalScope.worker._clPosts++; },
  _clPosts: 0,
} };
const W = globalThis.WorkerGlobalScope.worker;

const { loadMicroPython } = await import(pathToFileURL(mjsPath));
const mp = await loadMicroPython({ heapsize: 8 * 1024 * 1024, stdout: () => { stdoutCount++; } });

// Run a snippet, returning {ok, err}. Snippets assert internally.
const run = async (src) => {
  try { await mp.runPythonAsync(src); return { ok: true }; }
  catch (e) { return { ok: false, err: (e.message || String(e)).trim() }; }
};

// ----------------------------------------------------------------- machine
group("machine");
{
  W.input = 0;
  const r = await run(`
import machine
assert str(machine.Pin.board.CL0) == "Pin(0)", "board.CL0"
assert str(machine.Pin("LIGHT_SENSE")) == "Pin(43)", "Pin by name"
p = machine.PWM(machine.Pin.board.CL2)
p.freq(500)
p.duty_u16(32768)
assert p.duty_u16() == 32768, "duty readback"
assert machine.ADC(machine.Pin.board.VBAT_SENSE).read_u16() == 39700, "VBAT default"
assert len(machine.RTC().datetime()) == 8, "rtc datetime"
assert len(machine.unique_id()) == 8, "unique_id"
`);
  ok("machine API", r.ok, r.err);
  ok("PWM CL2 -> worker.caselights", Math.abs(W.machine.caselights[2] - 32768 / 65535) < 1e-6,
     `got ${W.machine.caselights[2]}`);

  W.input = 0b010000; // BUTTON_A held (active-low at the pin)
  const r2 = await run(`
import machine
assert machine.Pin.board.BUTTON_A.value() == 0, "A pressed -> 0"
assert machine.Pin.board.BUTTON_B.value() == 1, "B released -> 1"
`);
  ok("Pin.value reads worker.input", r2.ok, r2.err);
  W.input = 0;
}

// ------------------------------------------------------------------ st7789
group("st7789");
{
  const r = await run(`
import st7789, picovector
d = st7789.ST7789()
assert (d.WIDTH, d.HEIGHT) == (160, 120), "lores default"
d.fullres(True)
assert (d.WIDTH, d.HEIGHT) == (320, 240), "hires"
assert len(memoryview(d)) == 320 * 240 * 4, "framebuffer buffer protocol"
picovector.image(d.WIDTH, d.HEIGHT, memoryview(d))
d.backlight(0.5)
d.update()
`);
  ok("ST7789 drop-in API", r.ok, r.err);
  ok("backlight -> worker.backlight", Math.abs((W.backlight ?? -1) - 127 / 255) < 0.01,
     `got ${W.backlight}`);
}

// --------------------------------------------------- cooperative yield/input
group("cooperative yield (input mid-loop)");
{
  W.running = true; W.paused = false; W.input = 0;
  setTimeout(() => { W.input = 0b010000; }, 100);
  const r = await run(`
import machine, time
start = time.ticks_ms()
while True:
    if machine.Pin.board.BUTTON_A.value() == 0:
        break
    if time.ticks_ms() - start > 3000:
        raise RuntimeError("never saw button (yield failed)")
`);
  ok("tight loop sees button pushed from JS", r.ok, r.err);
  W.input = 0;
}

// -------------------------------------------------------------------- pause
group("pause (running && paused)");
{
  // stdout is the observable progress signal: a printing loop floods lines
  // while running and should stall completely while paused.
  W.running = true; W.paused = false;
  const printer = run(`
import time
start = time.ticks_ms()
while True:
    print(".")
    if time.ticks_ms() - start > 4000:
        break
`);
  await sleep(200);
  W.paused = true;
  await sleep(80);  const a = stdoutCount;     // let the in-flight iteration settle
  await sleep(300); const b = stdoutCount;     // fully paused window
  W.paused = false;
  await sleep(300); const c = stdoutCount;     // after resume
  ok("frozen while paused", (b - a) <= 1, `delta=${b - a}`);
  ok("resumes after unpause", (c - b) > 3, `delta=${c - b}`);
  W.running = false; W.paused = false;
  await printer;
}

// --------------------------------------------------------------- networking
group("networking (fetch-backed requests / urllib.urequest)");
{
  // Spin up a throwaway local HTTP server; the fetch-backed shims should reach
  // it through the browser/node `fetch` the _jsfetch primitive drives.
  const http = await import("node:http");
  const server = http.createServer((req, res) => {
    let body = "";
    req.on("data", (d) => { body += d; });
    req.on("end", () => {
      if (req.url === "/json") {
        res.setHeader("Content-Type", "application/json");
        res.end(JSON.stringify({ hello: "world", method: req.method }));
      } else if (req.url === "/echo") {
        res.end("echo:" + body);
      } else if (req.url === "/created") {
        res.statusCode = 201;
        res.end("created");
      } else {
        res.end("ok");
      }
    });
  });
  await new Promise((r) => server.listen(0, "127.0.0.1", r));
  const base = `http://127.0.0.1:${server.address().port}`;

  const r = await run(`
import requests, urllib.urequest, json
r = requests.get("${base}/json")
assert r.status_code == 200, r.status_code
assert r.headers.get("content-type", "").startswith("application/json"), r.headers
j = r.json()
assert j["hello"] == "world" and j["method"] == "GET", j

r2 = requests.post("${base}/echo", data="hi")
assert r2.status_code == 200, r2.status_code
assert r2.text == "echo:hi", r2.text

r3 = requests.get("${base}/created")
assert r3.status_code == 201, r3.status_code

resp = urllib.urequest.urlopen("${base}/json")
assert resp.status == 200, resp.status
assert json.loads(resp.read())["hello"] == "world"
resp.close()
`);
  ok("requests.get/post + urllib.urequest.urlopen via fetch", r.ok, r.err);

  // fetch.AsyncFetch (streaming shim): blocking fetch, the update() loop, and
  // streaming to a file.
  const portNum = server.address().port;
  const f = await run(`
import fetch, json
F = fetch.AsyncFetch

# blocking fetch to an in-memory buffer
api = F("127.0.0.1", ${portNum}, use_tls=False)
api.fetch("/json", blocking=True)
assert api.http_status == 200, api.http_status
assert api.to_json()["hello"] == "world", api.to_json()

# cooperative update()-driven fetch
api2 = F("127.0.0.1", ${portNum}, use_tls=False)
api2.fetch("/json")
while api2.update() not in (F.DONE, F.ERROR):
    pass
assert api2.status == F.DONE, api2.status
assert api2.to_json()["method"] == "GET", api2.to_json()

# streaming to a file
api3 = F("127.0.0.1", ${portNum}, use_tls=False)
api3.fetch("/echo", file="/fetch_out.txt", data="stream me", blocking=True)
with open("/fetch_out.txt") as fh:
    assert fh.read() == "echo:stream me", "file stream"
`);
  ok("fetch.AsyncFetch (blocking / update loop / to-file)", f.ok, f.err);

  // umqtt.simple is raw-TCP: frozen for source compatibility, not functional.
  const u = await run(`import umqtt.simple`);
  const notFrozen = !u.ok && (u.err.includes("no module named 'umqtt") );
  ok("umqtt.simple frozen (non-functional)", !notFrozen,
     notFrozen ? "not frozen" : "");
  server.close();
}

// --------------------------------------------------- networking (live HTTPS)
group("networking (live HTTPS fetch)");
{
  // A real request to a public, CORS-enabled HTTPS endpoint. Network failures
  // (offline / DNS / timeout) are skipped rather than failed so the suite stays
  // useful without connectivity; a reachable-but-wrong response still fails.
  const netErr = /fetch failed|getaddrinfo|ENOTFOUND|EAI_AGAIN|ECONNREFUSED|ETIMEDOUT|network|timed out|dns/i;
  const url = "https://pimoroni.github.io/feed2image/jokeapi-0.json";

  const r = await run(`
import requests
r = requests.get("${url}")
assert r.status_code == 200, "status %s" % r.status_code
j = r.json()
assert "joke" in j and "category" in j, j
`);
  if (r.ok) ok(`requests.get("${url}")`, true);
  else if (netErr.test(r.err)) skip(`requests.get("${url}")`, "network unavailable");
  else ok(`requests.get("${url}")`, false, r.err);

  // Same endpoint via the streaming AsyncFetch shim.
  const f = await run(`
import fetch
api = fetch.AsyncFetch("pimoroni.github.io", 443, use_tls=True)
api.fetch("/feed2image/jokeapi-0.json", blocking=True)
assert api.http_status == 200, api.http_status
j = api.to_json()
assert "joke" in j and "category" in j, j
`);
  if (f.ok) ok(`fetch.AsyncFetch -> ${url}`, true);
  else if (netErr.test(f.err)) skip(`fetch.AsyncFetch -> ${url}`, "network unavailable");
  else ok(`fetch.AsyncFetch -> ${url}`, false, f.err);
}

// ------------------------------------------------------------------ summary
console.log(`\n${fail === 0 ? "PASS" : "FAIL"}: ${pass} passed, ${fail} failed, ${skipped} skipped`);
process.exit(fail === 0 ? 0 : 1);
