const BOARD = 'Waveshare ESP32-S3-Touch-AMOLED-1.75';
const EXPECTED = new Map([['bootloader.bin', 0], ['partitions.bin', 32768], ['app.bin', 65536]]);
const SAVE_OFFSET = 0x610000;

export function validateRelease(release) {
  const fail = message => { throw new Error(message); };
  if (!release || release.schema !== 1 || release.product !== 'Pocket Wayfarer') fail('Metadati del firmware non validi.');
  if (!/^\d+\.\d+\.\d+(?:-[a-zA-Z0-9.]+)?$/.test(release.version)) fail('Versione non valida.');
  if (release.board !== BOARD || release.chip_family !== 'ESP32-S3' || release.flash_size !== 16777216) fail('Firmware per una scheda diversa.');
  if (release.channel !== 'preview' || release.hardware_tested !== false) fail('Stato di verifica non riconosciuto.');
  if (!/^[a-f0-9]{40}$/.test(release.source_commit) || typeof release.source_dirty !== 'boolean') fail('Identità del sorgente mancante.');
  if (!Number.isInteger(release.save_format) || release.save_format < 1) fail('Formato di salvataggio non valido.');
  if (release.manifest !== 'manifest.json' || release.bundle !== `pocketwayfarer-${release.version}.zip`) fail('Percorso del pacchetto non valido.');
  if (release.save_partition?.offset !== SAVE_OFFSET || release.save_partition?.size !== 0x40000 || release.app_partition?.offset !== 65536 || release.app_partition?.size !== 0x600000) fail('Layout delle partizioni non valido.');
  if (release.flash_settings?.flash_mode !== 'dio' || release.flash_settings?.flash_size !== '16MB' || release.flash_settings?.flash_freq !== '80m') fail('Configurazione flash non valida.');
  if (!Array.isArray(release.parts) || release.parts.length !== 3) fail('Pacchetto firmware incompleto.');
  const names = new Set();
  for (const part of release.parts) {
    if (!EXPECTED.has(part.path) || names.has(part.path) || part.offset !== EXPECTED.get(part.path)) fail('File o indirizzo flash inatteso.');
    if (!Number.isSafeInteger(part.size) || part.size <= 0 || !/^[a-f0-9]{64}$/.test(part.sha256)) fail('Integrità del file non dichiarata.');
    if (part.offset + part.size > SAVE_OFFSET) fail('Il pacchetto invade la partizione dei salvataggi.');
    names.add(part.path);
  }
  const sorted = [...release.parts].sort((a, b) => a.offset - b.offset);
  for (let i = 0; i < sorted.length - 1; i++) if (sorted[i].offset + sorted[i].size > sorted[i + 1].offset) fail('I file firmware si sovrappongono.');
  return release;
}

export async function sha256(bytes) {
  const digest = await globalThis.crypto.subtle.digest('SHA-256', bytes);
  return [...new Uint8Array(digest)].map(v => v.toString(16).padStart(2, '0')).join('');
}

export async function verifyParts(release, readFile) {
  validateRelease(release);
  const files = new Map();
  for (const part of release.parts) {
    const bytes = await readFile(part.path);
    if (bytes.byteLength !== part.size || await sha256(bytes) !== part.sha256) throw new Error(`Verifica SHA-256 fallita: ${part.path}.`);
    files.set(part.path, bytes);
  }
  const boot = new Uint8Array(files.get('bootloader.bin'));
  if (boot[0] !== 0xe9 || boot[2] !== 2 || boot[3] !== 0x4f || boot[12] !== 9 || boot[13] !== 0) throw new Error('Bootloader non compatibile con questa scheda.');
  const app = new Uint8Array(files.get('app.bin'));
  if (app.length < 80 || app[0] !== 0xe9 || app[12] !== 9 || app[13] !== 0 || new DataView(app.buffer).getUint32(32, true) !== 0xabcd5432) throw new Error('Applicazione non valida per ESP32-S3.');
  const version = new TextDecoder().decode(app.slice(48, 80)).split('\0')[0];
  if (version !== release.version) throw new Error('La versione nel binario non corrisponde al pacchetto.');
  const table = new Uint8Array(files.get('partitions.bin'));
  const partitions = new Map();
  for (let offset = 0; offset + 32 <= table.length; offset += 32) {
    const view = new DataView(table.buffer, offset, 32);
    if (view.getUint16(0, true) !== 0x50aa) break;
    const name = new TextDecoder().decode(table.slice(offset + 12, offset + 28)).split('\0')[0];
    if (partitions.has(name)) throw new Error('Etichetta partizione duplicata.');
    partitions.set(name, {offset: view.getUint32(4, true), size: view.getUint32(8, true), type: view.getUint8(2), subtype: view.getUint8(3), flags: view.getUint32(28, true)});
  }
  if (partitions.get('factory')?.offset !== 65536 || partitions.get('factory')?.size !== 0x600000 || partitions.get('wf_nvs')?.offset !== SAVE_OFFSET || partitions.get('wf_nvs')?.size !== 0x40000) throw new Error('La tabella partizioni non corrisponde al layout supportato.');
  const expectedPartitions = [['nvs', 1, 2, 0x9000, 0x6000], ['phy_init', 1, 1, 0xf000, 0x1000], ['factory', 0, 0, 0x10000, 0x600000], ['wf_nvs', 1, 2, SAVE_OFFSET, 0x40000]];
  if (partitions.size !== expectedPartitions.length || expectedPartitions.some(([name, type, subtype, offset, size]) => {
    const p = partitions.get(name);
    return !p || p.type !== type || p.subtype !== subtype || p.offset !== offset || p.size !== size || p.flags !== 0;
  })) throw new Error('La tabella partizioni contiene una configurazione inattesa.');
  return files;
}

export function verifiedManifest(release, files, createURL) {
  validateRelease(release);
  return {
    name: 'Pocket Wayfarer', version: release.version,
    new_install_prompt_erase: true, new_install_improv_wait_time: 0,
    builds: [{ chipFamily: 'ESP32-S3', parts: release.parts.map(part => {
      const bytes = files.get(part.path);
      if (!bytes || bytes.byteLength !== part.size) throw new Error('File verificato mancante.');
      return {path: createURL(new Blob([bytes], {type: 'application/octet-stream'})), offset: part.offset};
    }) }],
  };
}

async function startInstaller() {
  const $ = id => document.getElementById(id);
  const status = (text, state = 'waiting') => { $('install-status').textContent = text; $('install-status').dataset.state = state; };
  const base = new URL('firmware/', document.baseURI);
  const blobs = [];
  let prepared, release;
  const objectURL = blob => { const url = URL.createObjectURL(blob); blobs.push(url); return url; };
  addEventListener('pagehide', () => blobs.forEach(url => URL.revokeObjectURL(url)), {once: true});
  $('retry-verify').addEventListener('click', () => location.reload());
  try {
    const response = await fetch(new URL('release.json', base), {cache: 'no-cache'});
    if (!response.ok) throw new Error('Pacchetto firmware non disponibile.');
    release = validateRelease(await response.json());
    $('release-version').textContent = `${release.version}${release.source_dirty ? ' · build locale' : ' · Lanternbound'}`;
    $('save-format').textContent = `v${release.save_format} · migrazione dei formati precedenti`;
    $('source-commit').textContent = `${release.source_commit.slice(0, 8)}${release.source_dirty ? ' + modifiche locali' : ''}`;
    $('download-bundle').href = new URL(release.bundle, base).href; $('download-bundle').hidden = false;
    $('download-hashes').href = new URL('SHA256SUMS.txt', base).href; $('download-hashes').hidden = false;
    if (!globalThis.isSecureContext) {
      status('Per accedere alla USB apri il sito tramite HTTPS, oppure usa localhost.', 'unsupported');
      return;
    }
    if (!('serial' in navigator)) {
      status('Apri questa pagina da Chrome o Edge su un computer con Web Serial. Qui puoi comunque leggere la guida e scaricare il pacchetto.', 'unsupported');
      return;
    }
    $('device-confirm').disabled = false;
    status('Controlla il modello e spunta la conferma per preparare l’installazione.');
    $('device-confirm').addEventListener('change', async () => {
      $('web-installer').hidden = true;
      if (!$('device-confirm').checked) { status('Conferma il modello per continuare.'); return; }
      try {
        status('Verifico i tre file del firmware…');
        $('integrity-status').textContent = 'Verifica in corso';
        prepared ??= (async () => {
          const files = await verifyParts(release, async path => {
            const response = await fetch(new URL(path, base), {cache: 'force-cache'});
            if (!response.ok) throw new Error(`File firmware non disponibile: ${path}.`);
            return response.arrayBuffer();
          });
          const manifest = verifiedManifest(release, files, objectURL);
          // Pass the verified bytes to ESP Web Tools via Blob URLs, avoiding
          // a second network fetch between integrity checks and actual flashing.
          const manifestURL = objectURL(new Blob([JSON.stringify(manifest)], {type: 'application/json'}));
          await import('../vendor/esp-web-tools/install-button.js');
          await customElements.whenDefined('esp-web-install-button');
          $('web-installer').manifest = manifestURL;
        })();
        await prepared;
        $('integrity-status').textContent = '3 file · SHA-256 verificati';
        if ($('device-confirm').checked) {
          $('web-installer').hidden = false;
          status('Pacchetto verificato. Premi il pulsante per scegliere la porta USB.', 'ready');
        }
      } catch (error) {
        $('web-installer').hidden = true;
        $('retry-verify').hidden = false;
        $('integrity-status').textContent = 'Verifica non superata';
        status(`${error.message} L’installazione resta disattivata.`, 'error');
      }
    });
  } catch (error) {
    $('device-confirm').disabled = true;
    $('retry-verify').hidden = false;
    $('release-version').textContent = 'Firmware non disponibile';
    status(`${error.message} Consulta la guida o riprova più tardi.`, 'error');
  }
}

if (typeof document !== 'undefined') startInstaller();
