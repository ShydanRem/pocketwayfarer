import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {resolve} from 'node:path';
import {validateRelease, verifyParts, verifiedManifest, sha256} from '../../web/js/install.mjs';

const root = resolve(process.env.POCKET_TEST_SITE ?? 'build/site', 'firmware');
const release = JSON.parse(await readFile(resolve(root, 'release.json'), 'utf8'));
const clone = () => structuredClone(release);
const read = async name => {
  const bytes = await readFile(resolve(root, name));
  return bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
};

test('actual packaged firmware verifies chip, embedded version, all hashes and save layout', async () => {
  const files = await verifyParts(release, read);
  assert.equal(files.size, 3);
  assert.equal(files.get('app.bin').byteLength, release.parts.find(p => p.path === 'app.bin').size);
});

test('unsupported board, version/path injection, missing parts, overlaps and wrong layout fail before flashing', () => {
  for (const alter of [
    r => r.board = 'ESP32-S3 generic', r => r.version = '../app.bin', r => r.parts.pop(),
    r => r.parts[0].path = 'https://example.invalid/fw.bin', r => r.parts[1].offset = 0,
    r => r.parts[0].size = 32769, r => r.parts[2].size = 0x610000,
    r => r.save_partition.offset = 0x300000, r => r.hardware_tested = true,
  ]) {
    const r = clone(); alter(r); assert.throws(() => validateRelease(r));
  }
});

test('truncated or modified bytes are rejected even if the HTTP request succeeded', async () => {
  await assert.rejects(verifyParts(release, async name => (await read(name)).slice(1)), /SHA-256/);
  await assert.rejects(verifyParts(release, async name => {
    const bytes = new Uint8Array(await read(name)); bytes[20] ^= 1; return bytes.buffer;
  }), /SHA-256/);
});

test('self-consistent hashes cannot hide a wrong chip, app version or save partition table', async () => {
  for (const [name, offset] of [['bootloader.bin', 12], ['app.bin', 48], ['partitions.bin', 4]]) {
    const r = clone(), bytes = new Uint8Array(await read(name));
    bytes[offset] ^= 1;
    r.parts.find(p => p.path === name).sha256 = await sha256(bytes);
    await assert.rejects(verifyParts(r, file => file === name ? Promise.resolve(bytes.buffer) : read(file)));
  }
});

test('manifest uses verified Blob bytes, correct offsets, erase opt-in and no Wi-Fi provisioning', async () => {
  const files = await verifyParts(release, read), blobs = [];
  const manifest = verifiedManifest(release, files, blob => { blobs.push(blob); return `blob:test-${blobs.length}`; });
  assert.equal(manifest.new_install_prompt_erase, true);
  assert.equal(manifest.new_install_improv_wait_time, 0);
  assert.equal(manifest.builds[0].chipFamily, 'ESP32-S3');
  assert.deepEqual(manifest.builds[0].parts.map(p => p.offset), [0, 32768, 65536]);
  for (let i = 0; i < blobs.length; i++) assert.equal(await sha256(await blobs[i].arrayBuffer()), release.parts[i].sha256);
});
