"""Pinned Qt binaries from the official installer repository (architecture-split layout)."""
import argparse
import concurrent.futures
import hashlib
import json
import pathlib
import re
import sys
import urllib.request
import xml.etree.ElementTree as ET
import py7zr

VERSION = '6.12.0'
ARCHES = {
    'windows': ('windows_x86', 'msvc2022_64', 'win64_msvc2022_64', 'msvc2022_64'),
    'linux': ('linux_x64', 'gcc_64', 'linux_gcc_64', 'gcc_64'),
    'macos': ('mac_x64', 'clang_64', 'clang_64', 'macos'),
}
def get(url):
    with urllib.request.urlopen(url, timeout=120) as r:
        return r.read()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', choices=ARCHES, default='windows' if sys.platform=='win32' else 'macos' if sys.platform=='darwin' else 'linux')
    parser.add_argument('--output', type=pathlib.Path, default=pathlib.Path('.tools/qt'))
    args = parser.parse_args()
    host, folder, architecture, prefix = ARCHES[args.host]
    base = f'https://download.qt.io/online/qtsdkrepository/{host}/desktop/qt6_6120/'
    # Qt 6.12 split metadata by target architecture; older aqt releases assume one XML.
    index = get(base).decode()
    candidates = re.findall(r'href="([^"?#]+/)"', index)
    match = next((x for x in candidates if folder in x and 'debug' not in x), None)
    if not match and 'qt6_6120/' in candidates:
        match = 'qt6_6120/'
    if not match:
        raise RuntimeError(f'Official Qt repository does not contain {folder}')
    base += match
    root = ET.fromstring(get(base+'Updates.xml'))
    wanted = {'qtbase','qtdeclarative','qtsvg','qttools','qttranslations','qtshadertools','qtwayland'}
    jobs = []
    for package in root.findall('PackageUpdate'):
        name = package.findtext('Name','')
        if not name.endswith('.'+architecture) or '.debug' in name:
            continue
        for archive in package.findtext('DownloadableArchives','').split(','):
            archive = archive.strip()
            if archive.split('-')[0] not in wanted and not archive.startswith(('d3dcompiler_', 'opengl32sw-', 'icu-linux-')):
                continue
            if any(x[1]==archive for x in jobs):
                continue
            url = base+name+'/'+package.findtext('Version')+archive
            jobs.append((url,archive))
    if not {'qtbase','qtdeclarative'}.issubset({x[1].split('-')[0] for x in jobs}):
        raise RuntimeError('Missing Qt base/declarative packages')
    cache=args.output.parent/'downloads'; cache.mkdir(parents=True,exist_ok=True)
    args.output.mkdir(parents=True,exist_ok=True)
    def fetch(job):
        url,name=job
        expected=get(url+'.sha1').decode().strip().split()[0].lower()
        path=cache/name
        if not path.exists() or hashlib.sha1(path.read_bytes()).hexdigest()!=expected:
            data=get(url)
            if hashlib.sha1(data).hexdigest()!=expected:
                raise RuntimeError('Qt archive integrity mismatch: '+name)
            path.write_bytes(data)
        return path, {'url':url,'sha1':expected,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        results=list(pool.map(fetch,jobs))
    for path,record in results:
        print('Extracting',path.name,flush=True)
        with py7zr.SevenZipFile(path) as archive:
            for name in archive.getnames():
                target=(args.output/name).resolve()
                if not target.is_relative_to(args.output.resolve()):
                    raise RuntimeError('Unsafe Qt archive path')
            archive.extractall(args.output)
    qt=args.output
    if args.host == 'linux':
        # Qt's ICU archive is flat, while ELF dependencies resolve from lib/.
        (qt/'lib').mkdir(exist_ok=True)
        import shutil
        for library in qt.glob('libicu*.so*'):
            shutil.copy2(library,qt/'lib'/library.name)
    # Relocate using Qt's supported qt.conf; CMake configs derive their prefix.
    (qt/'bin'/'qt.conf').write_text('[Paths]\nPrefix=..\n',encoding='utf-8')
    if args.host != 'windows':
        for folder in ('bin','libexec'):
            for executable in (qt/folder).rglob('*'):
                if executable.is_file():
                    executable.chmod(executable.stat().st_mode | 0o111)
    (args.output/'download-manifest.json').write_text(json.dumps([r for _,r in results],indent=2))
    print('Qt installed:',qt.resolve(),flush=True)
if __name__=='__main__':
    main()
