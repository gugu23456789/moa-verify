// browser_smoke_test.mjs — playwright smoke test for the moa-verify page.
//
// Loads the served page, waits for the in-page verdict, and asserts the browser
// verified the wasm/input fingerprints and reproduced the reference bytes.
// Usage: node browser_smoke_test.mjs http://127.0.0.1:4173
import { chromium } from 'playwright';

const base = process.argv[2] || 'http://127.0.0.1:4173';
const browser = await chromium.launch();
const page = await browser.newPage();

const pageErrors = [];
page.on('pageerror', e => pageErrors.push(String(e)));

await page.goto(base + '/index.html', { waitUntil: 'load' });
await page.waitForFunction(() => window.__moaVerdict !== undefined, null, { timeout: 20000 });

const verdict = await page.evaluate(() => window.__moaVerdict);
const status = (await page.textContent('#status')) || '';
await browser.close();

if (pageErrors.length) {
  console.error('FAIL: page errors:\n' + pageErrors.join('\n'));
  process.exit(1);
}
if (!verdict || verdict.ok !== true || verdict.reason !== 'verified') {
  console.error('FAIL: browser verdict = ' + JSON.stringify(verdict));
  console.error('status: ' + status.trim());
  process.exit(1);
}
console.log('OK: browser verified fingerprints and reproduced reference bytes');
console.log('  ' + status.trim());
