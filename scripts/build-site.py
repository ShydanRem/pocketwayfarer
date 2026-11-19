#!/usr/bin/env python3
"""Assemble the static GitHub Pages site from actual screenshots and firmware."""
import argparse
import hashlib
import importlib.util
import html
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'build/site-tools'))
import markdown
SCREENS = {
    'borgo': '02-borgo', 'percorso': '04-percorso', 'combattimento': '05-combattimento',
    'reliquia': '10-reliquia', 'guida': '32-guida-primo-attacco', 'sentinella': '03-assetto',
    'piromante': '25-piromante', 'predone': '26-predone', 'parata': '35-parata-perfetta-scudo',
    'risonanza': '39-risonanza-impatto', 'vittoria': '16-vittoria', 'opzioni': '27-options-english',
    'titolo': '01-titolo', 'panoramica': 'panoramica', 'guida-e-risonanza': 'guida-e-risonanza',
    'assetti-e-guardiano': 'assetti-e-guardiano',
    'confronto-reliquie': '47-confronto-reliquie',
}
CLASS_SCREENS = {'classi': 'otto-classi-base', 'arciere': '03-classe-02', 'classi-avanzate-a':'otto-classi-avanzate-a', 'classi-avanzate-b':'otto-classi-avanzate-b', 'classi-maestre-a':'otto-classi-maestre-a', 'classi-maestre-b':'otto-classi-maestre-b'}
RIG_SCREENS = {'avatar-base': 'otto-avatar-base', 'avatar-avanzate-a':'otto-avatar-avanzate-a', 'avatar-avanzate-b':'otto-avatar-avanzate-b', 'avatar-maestre-a':'otto-avatar-maestre-a', 'avatar-maestre-b':'otto-avatar-maestre-b'}
RIG_SCREENS['interfaccia-classico'] = 'interfaccia-classico'


def manual_html():
    sections = []
    aliases = {'GUIDA.md': '#inizio', 'INSTALLAZIONE.md': '#installazione', 'SALVATAGGI.md': '#salvataggi', 'RISOLUZIONE-PROBLEMI.md': '#problemi', 'SVILUPPO.md': '#desktop'}
    reverse_images = {source + '.png': name + '.png' for name, source in SCREENS.items()}
    for anchor, name in [('inizio', 'GUIDA.md'), ('installazione', 'INSTALLAZIONE.md'), ('salvataggi', 'SALVATAGGI.md'), ('problemi', 'RISOLUZIONE-PROBLEMI.md'), ('desktop', 'SVILUPPO.md')]:
        text = (ROOT / 'docs' / name).read_text()
        if anchor == 'desktop':
            text = '# Provare Pocket Wayfarer su PC\n\n' + text.split('## Provare la UI su Linux', 1)[1].split('## Test e immagini native', 1)[0]
        rendered = markdown.markdown(text, extensions=['tables', 'fenced_code', 'toc'])
        rendered = re.sub(r'<(/?)h([1-5])(\b)', lambda m: f'<{m[1]}h{int(m[2])+1}{m[3]}', rendered)
        for original, alias in reverse_images.items():
            rendered = rendered.replace('src="img/lantern/' + original + '"', 'src="assets/screens/' + alias + '" loading="lazy"')
        for alias,original in CLASS_SCREENS.items():
            rendered = rendered.replace('src="img/classes/'+original+'.png"', 'src="assets/screens/'+alias+'.png" loading="lazy"')
        for alias, original in RIG_SCREENS.items():
            rendered = rendered.replace('src="img/hero_rig/'+original+'.png"', 'src="assets/screens/'+alias+'.png" loading="lazy"')
        for original, alias in aliases.items():
            rendered = re.sub(r'href="' + re.escape(original) + r'(?:#[^"]*)?"', 'href="' + alias + '"', rendered)
        rendered = rendered.replace('id="installazione-manuale"', 'id="manuale"').replace('id="la-porta-usb-non-compare"', 'id="connessione"')
        rendered = re.sub(r'href="(FIRMWARE|LANTERNBOUND|ONBOARDING-FEEDBACK|ART-CLARITY|DEVICE-AUDIT-2026-11-17)\.md(?:#[^"]*)?"', lambda m: 'href="downloads/' + m[1] + '.md"', rendered)
        rendered = rendered.replace('<table>', '<div class="table-wrap"><table>').replace('</table>', '</table></div>')
        def image_figure(match):
            tag = match[1]
            alt = re.search(r'alt="([^"]*)"', tag)[1]
            wide = any(name in tag for name in ['panoramica.png', 'guida-e-risonanza.png', 'assetti-e-guardiano.png', 'interfaccia-classico.png'])
            return '<figure' + (' class="wide"' if wide else '') + '>' + tag + '<figcaption>' + alt + '</figcaption></figure>'
        rendered = re.sub(r'<p>(<img [^>]+>)</p>', image_figure, rendered)
        sections.append(f'<section id="{anchor}">{rendered}</section>')
    return '\n'.join(sections)


def roadmap_html():
    body = markdown.markdown((ROOT/'docs/ROADMAP.md').read_text(),extensions=['tables','fenced_code','toc'])
    body = re.sub(r'<(/?)h([1-5])(\b)', lambda m:f'<{m[1]}h{int(m[2])+1}{m[3]}',body)
    commit = subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    def source_link(match):
        path=match[1]
        if re.match(r'^[a-z]+:|#',path): return match[0]
        resolved=(ROOT/'docs'/path).resolve()
        assert resolved.is_relative_to(ROOT)
        assert resolved.exists(), f'Missing roadmap source: {path}'
        return 'href="https://github.com/ShydanRem/pocketwayfarer/blob/'+commit+'/'+resolved.relative_to(ROOT).as_posix()+'"'
    body=re.sub(r'href="([^"]+)"',source_link,body)
    body=body.replace('src="img/classes/otto-classi-base.png"','src="assets/screens/classi.png" loading="lazy"')
    body=body.replace('src="img/hero_rig/otto-avatar-base.png"','src="assets/screens/avatar-base.png" loading="lazy"')
    body=body.replace('src="img/hero_rig/otto-avatar-avanzate-a.png"','src="assets/screens/avatar-avanzate-a.png" loading="lazy"')
    body=body.replace('src="img/classes/otto-classi-avanzate-a.png"','src="assets/screens/classi-avanzate-a.png" loading="lazy"')
    body=body.replace('src="img/hero_rig/otto-avatar-avanzate-b.png"','src="assets/screens/avatar-avanzate-b.png" loading="lazy"')
    body=body.replace('src="img/classes/otto-classi-avanzate-b.png"','src="assets/screens/classi-avanzate-b.png" loading="lazy"')
    body=body.replace('src="img/hero_rig/otto-avatar-maestre-a.png"','src="assets/screens/avatar-maestre-a.png" loading="lazy"')
    body=body.replace('src="img/classes/otto-classi-maestre-a.png"','src="assets/screens/classi-maestre-a.png" loading="lazy"')
    body=body.replace('src="img/hero_rig/otto-avatar-maestre-b.png"','src="assets/screens/avatar-maestre-b.png" loading="lazy"')
    body=body.replace('src="img/classes/otto-classi-maestre-b.png"','src="assets/screens/classi-maestre-b.png" loading="lazy"')
    body=body.replace('src="img/hero_rig/interfaccia-classico.png"','src="assets/screens/interfaccia-classico.png" loading="lazy"')
    return body.replace('<table>','<div class="table-wrap"><table>').replace('</table>','</table></div>')


def notices_html():
    sections = []
    for title, path in [('ESP Web Tools — Apache-2.0', ROOT / 'web/vendor/esp-web-tools/LICENSE'),
                        ('Dipendenze dell’installer — avvisi originali', ROOT / 'web/vendor/esp-web-tools/THIRD_PARTY_NOTICES.md'),
                        ('DejaVu — font del titolo', ROOT / 'assets/lantern/fonts/LICENSE.txt')]:
        sections.append(f'<section><h2>{html.escape(title)}</h2><details><summary>Leggi gli avvisi completi</summary><pre>{html.escape(path.read_text())}</pre></details></section>')
    return '\n'.join(sections)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--firmware-build', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/site')
    parser.add_argument('--allow-dirty', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / 'build') or output == ROOT / 'build':
        raise SystemExit('Site output must be a subdirectory under build/')
    subprocess.run(['python3', str(ROOT / 'scripts/vendor-web-tools.py'), '--check'], check=True)
    output.mkdir(parents=True, exist_ok=True)
    shutil.copytree(ROOT / 'web', output, dirs_exist_ok=True)
    for template in output.glob('*.template.html'):
        template.unlink()
    screens = output / 'assets/screens'; screens.mkdir(parents=True, exist_ok=True)
    images = {}
    for name, source in SCREENS.items():
        original = ROOT / 'docs/img/lantern' / (source + '.png')
        shutil.copyfile(original, screens / (name + '.png'))
        images[name + '.png'] = dict(source=original.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(original.read_bytes()).hexdigest())
    for name,source in CLASS_SCREENS.items():
        original=ROOT/'docs/img/classes'/(source+'.png')
        shutil.copyfile(original,screens/(name+'.png'))
        images[name+'.png']=dict(source=original.relative_to(ROOT).as_posix(),sha256=hashlib.sha256(original.read_bytes()).hexdigest())
    for name,source in RIG_SCREENS.items():
        original=ROOT/'docs/img/hero_rig'/(source+'.png')
        shutil.copyfile(original,screens/(name+'.png'))
        images[name+'.png']=dict(source=original.relative_to(ROOT).as_posix(),sha256=hashlib.sha256(original.read_bytes()).hexdigest())
    fonts = output / 'assets/fonts'; fonts.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(ROOT / 'assets/lantern/fonts/title.ttf', fonts / 'title.ttf')
    shutil.copyfile(ROOT / 'assets/lantern/fonts/LICENSE.txt', fonts / 'LICENSE.txt')
    for name in ['INSTALLAZIONE.md', 'GUIDA.md', 'SALVATAGGI.md', 'RISOLUZIONE-PROBLEMI.md', 'SVILUPPO.md', 'PUBBLICAZIONE.md', 'FIRMWARE.md', 'LANTERNBOUND.md', 'ONBOARDING-FEEDBACK.md', 'ART-CLARITY.md', 'DEVICE-AUDIT-2026-11-17.md']:
        dest = output / 'downloads' / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / 'docs' / name, dest)
    (output / 'assets/image-sources.json').write_text(json.dumps(images, indent=2) + '\n')
    (output / '.nojekyll').touch()
    spec = importlib.util.spec_from_file_location('package_firmware', ROOT / 'scripts/package-firmware.py')
    package = importlib.util.module_from_spec(spec); spec.loader.exec_module(package)
    package.build_package(args.firmware_build.resolve(), output / 'firmware', args.allow_dirty)
    for page, body in [('guide', manual_html()), ('notices', notices_html()), ('roadmap',roadmap_html())]:
        template = (ROOT / 'web' / (page + '.template.html')).read_text()
        (output / (page + '.html')).write_text(template.replace('{{CONTENT}}', body).replace('{{VERSION}}', (ROOT / 'VERSION').read_text().strip()))
    # Only local preview configuration; this is a GitHub Pages project, not a
    # separately registered Sites deployment. Kept outside tracked source.
    preview = ROOT / '.openai'; preview.mkdir(exist_ok=True)
    (preview / 'hosting.json').write_text(json.dumps({'static': {'directory': output.relative_to(ROOT).as_posix()}}) + '\n')
    print(f'Static site ready: {output}')


if __name__ == '__main__':
    main()
