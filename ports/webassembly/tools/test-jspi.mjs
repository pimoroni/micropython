// Generic smoke test for the experimental `jspi` variant (no simulator, no
// worker). Confirms the JSPI entry points work and the cooperative VM yield
// (MICROPY_ENABLE_VM_YIELD) lets a tight Python loop hand the event loop a
// turn — verified by a setTimeout that fires *during* the loop, not after it.
//
// Usage: node tools/test-jspi.mjs [path/to/micropython.mjs]
//        (defaults to ../build-jspi/micropython.mjs)

import { fileURLToPath, pathToFileURL } from "node:url";
import { dirname, resolve } from "node:path";

const __dirname = dirname(fileURLToPath(import.meta.url));
const mjsPath = process.argv[2]
  ? resolve(process.argv[2])
  : resolve(__dirname, "../build-jspi/micropython.mjs");

let pass = 0, fail = 0;
const ok = (name, cond, detail = "") => {
  if (cond) { pass++; console.log(`  ok   ${name}`); }
  else { fail++; console.log(`  FAIL ${name}${detail ? "  — " + detail : ""}`); }
};

const { loadMicroPython } = await import(pathToFileURL(mjsPath));
const mp = await loadMicroPython({ heapsize: 4 * 1024 * 1024, stdout: () => {} });

// runPythonAsync resolves under JSPI.
const r = await mp.runPythonAsync("21 * 2");
ok("runPythonAsync works", true, String(r));

// A compute-bound loop with no I/O must still yield: a 100ms timer scheduled
// before it should fire well before the loop's ~600ms end.
let firedAt = -1;
const start = Date.now();
setTimeout(() => { if (firedAt < 0) firedAt = Date.now() - start; }, 100);
await mp.runPythonAsync(`
import time
s = time.ticks_ms()
while time.ticks_ms() - s < 600:
    pass
`);
const loopMs = Date.now() - start;
ok(`tight loop yields to event loop (timer fired @${firedAt}ms, loop ${loopMs}ms)`,
   firedAt >= 0 && firedAt < 300);

// time.sleep() should also yield (not busy-spin) — schedule another timer.
firedAt = -1;
const start2 = Date.now();
setTimeout(() => { if (firedAt < 0) firedAt = Date.now() - start2; }, 80);
await mp.runPythonAsync("import time\ntime.sleep(0.5)");
ok(`time.sleep yields (timer fired @${firedAt}ms)`, firedAt >= 0 && firedAt < 300);

// _jsfetch (enabled in this variant via jsfetch/jsfetch.mk): a blocking GET to
// a throwaway local server, suspending via the event loop until it completes.
const http = await import("node:http");
const server = http.createServer((req, res) => {
  res.setHeader("Content-Type", "application/json");
  res.end(JSON.stringify({ hello: "world" }));
});
await new Promise((r) => server.listen(0, "127.0.0.1", r));
const base = `http://127.0.0.1:${server.address().port}`;
try {
  await mp.runPythonAsync(`
import _jsfetch
status, reason, headers, body = _jsfetch.request("GET", "${base}/", "", None)
assert status == 200, status
assert b'"hello"' in body, body
`);
  ok("_jsfetch.request (blocking GET)", true);
} catch (e) {
  ok("_jsfetch.request (blocking GET)", false, (e.message || String(e)).split("\n").pop());
}
server.close();

console.log(`\n${fail === 0 ? "PASS" : "FAIL"}: ${pass} passed, ${fail} failed`);
process.exit(fail === 0 ? 0 : 1);
