// wasm_smoke_test.mjs — node smoke test for the WASM core.
//
// Loads the Emscripten glue directly (no browser), recomputes the sealed batch,
// and byte-compares to the published reference.  Usage:
//   node wasm_smoke_test.mjs <siteDir>
import { createRequire } from 'node:module';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const require = createRequire(import.meta.url);
const here = path.dirname(fileURLToPath(import.meta.url));

const site = path.resolve(process.argv[2] || path.join(here, '..', 'build', 'site'));
const SEALED = path.resolve(process.argv[3] || path.join(here, '..', 'data', 'moa_batch.csv'));

const EXPECTED =
  '{"arms":6,"per":{' +
  '"scope":{"mean":63.17,"min":45,"max":78},' +
  '"evidence":{"mean":69.00,"min":62,"max":78},' +
  '"assumptions":{"mean":45.33,"min":25,"max":62},' +
  '"recovery":{"mean":39.83,"min":28,"max":58},' +
  '"references":{"mean":68.33,"min":55,"max":82},' +
  '"bias":{"mean":62.17,"min":45,"max":82},' +
  '"cost":{"mean":51.83,"min":40,"max":74}},' +
  '"batch_mean":57.10,"consensus":{"not_landable":6}}';

const createModule = require(path.resolve(site, 'moa_verify.js'));
const csv = readFileSync(SEALED, 'utf8');

const mod = await createModule();
const actual = mod.verifyCsv(csv);
if (actual !== EXPECTED) {
  console.error('FAIL: wasm recompute != reference\n  got =' + actual + '\n  want=' + EXPECTED);
  process.exit(1);
}

// 负臂：篡改分数必须改变输出（防"忽略输入"的假实现）。
const tampered = csv.replace(',62,78,55,58,', ',1,78,55,58,');
if (mod.verifyCsv(tampered) === EXPECTED) {
  console.error('FAIL: tampered input produced the reference output');
  process.exit(1);
}

console.log('OK: wasm recompute bytes == reference (' + actual.length + ' bytes); tamper detected');
