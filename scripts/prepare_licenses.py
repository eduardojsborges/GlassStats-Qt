"""Provide exact Qt sources and extract their license/notices for binary distributions."""
import argparse
import hashlib
import pathlib
import shutil
import tarfile
import urllib.request

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=pathlib.Path,required=True)
    parser.add_argument('--sources-output',type=pathlib.Path)
    args=parser.parse_args()
    filename='qt-everywhere-src-6.12.0.tar.xz'
    url='https://download.qt.io/official_releases/qt/6.12/6.12.0/single/'+filename
    cache=pathlib.Path('.tools/downloads');cache.mkdir(parents=True,exist_ok=True)
    archive=cache/filename
    with urllib.request.urlopen(url+'.sha256',timeout=60) as response:
        expected=response.read().decode().split()[0].lower()
    def digest(path):
        h=hashlib.sha256()
        with path.open('rb') as f:
            for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
        return h.hexdigest()
    if not archive.exists() or digest(archive)!=expected:
        print('Downloading corresponding Qt source archive',flush=True)
        with urllib.request.urlopen(url,timeout=180) as response,archive.with_suffix('.partial').open('wb') as output:
            shutil.copyfileobj(response,output)
        if digest(archive.with_suffix('.partial'))!=expected:raise RuntimeError('Qt source SHA-256 mismatch')
        archive.with_suffix('.partial').replace(archive)
    args.output.mkdir(parents=True,exist_ok=True)
    with tarfile.open(archive,'r:xz') as tar:
        for item in tar:
            parts=pathlib.PurePosixPath(item.name).parts
            if not item.isfile() or '..' in parts:continue
            name=parts[-1].lower()
            if 'LICENSES' not in parts and not name.startswith(('license','copying','copyright','notice')) and name!='qt_attribution.json':continue
            relative=pathlib.Path(*parts[1:])
            if relative.parts and relative.parts[0] not in ('qtbase','qtdeclarative','qtsvg','qtshadertools','qttools','qttranslations','qtwayland'):continue
            destination=args.output/relative;destination.parent.mkdir(parents=True,exist_ok=True)
            with tar.extractfile(item) as source,destination.open('wb') as output:shutil.copyfileobj(source,output)
    (args.output/'SOURCE.txt').write_text(f'Qt 6.12.0 corresponding source\n{filename}\n{url}\nSHA256 {expected}\nThe exact archive accompanies the development artifacts / public binary release.\n',encoding='utf-8')
    if args.sources_output:
        args.sources_output.mkdir(parents=True,exist_ok=True)
        shutil.copy2(archive,args.sources_output/filename)
        (args.sources_output/(filename+'.sha256')).write_text(expected+'  '+filename+'\n')
    print('Qt license and source verification complete',flush=True)
if __name__=='__main__':main()
