import argparse
import hashlib
import os
import pathlib
import platform
import shutil
import subprocess
import urllib.request

VERSION='0.2.0-alpha.1'
ROOT=pathlib.Path(__file__).resolve().parent.parent
def run(*args,**kwargs):subprocess.run([str(a) for a in args],check=True,**kwargs)
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--platform',choices=['linux','macos'],required=True);parser.add_argument('--arch',choices=['x64','arm64'],default='x64');args=parser.parse_args()
    os.chdir(ROOT);artifacts=ROOT/'artifacts';artifacts.mkdir(exist_ok=True);qt=ROOT/'.tools/qt'
    if args.platform=='macos':
        staging=artifacts/('macos-'+args.arch);run('cmake','--install','build','--prefix',staging)
        bundle=staging/'GlassStats.app';run(qt/'bin/macdeployqt',bundle,'-qmldir='+str(ROOT/'qml'),'-always-overwrite','-no-codesign')
        resources=bundle/'Contents/Resources';resources.mkdir(parents=True,exist_ok=True)
        for filename in ('LICENSE','README.md','VALIDATION.md','CHANGELOG.md'):shutil.copy2(ROOT/filename,resources/filename)
        shutil.copytree(ROOT/'licenses',resources/'licenses',dirs_exist_ok=True)
        run(os.sys.executable,'scripts/prepare_licenses.py','--output',resources/'licenses/Qt')
        # Ad-hoc signing is for bundle integrity, not notarization or trusted publisher identity.
        run('codesign','--force','--deep','--sign','-',bundle)
        image=artifacts/f'GlassStats-Qt-{VERSION}-macos-{args.arch}.dmg'
        if image.exists():image.unlink()
        applications=staging/'Applications'
        if not applications.exists():applications.symlink_to('/Applications',target_is_directory=True)
        run('hdiutil','create','-volname','GlassStats Qt Preview','-srcfolder',staging,'-ov','-format','UDZO',image)
    else:
        appdir=artifacts/'AppDir';usr=appdir/'usr';usr.mkdir(parents=True,exist_ok=True)
        shutil.copy2(ROOT/'build/GlassStats',usr/'GlassStats')
        shutil.copytree(qt/'qml',usr/'qml',dirs_exist_ok=True)
        for folder in ('platforms','platforminputcontexts','xcbglintegrations','wayland-decoration-client','wayland-graphics-integration-client','wayland-shell-integration','imageformats','iconengines','networkinformation','tls'):
            if (qt/'plugins'/folder).exists():shutil.copytree(qt/'plugins'/folder,usr/'plugins'/folder,dirs_exist_ok=True)
        lib=usr/'lib';lib.mkdir(exist_ok=True);scan=[usr/'GlassStats']+list((usr/'qml').rglob('*.so'))+list((usr/'plugins').rglob('*.so'));seen=set()
        while scan:
            binary=scan.pop()
            if binary in seen:continue
            seen.add(binary)
            env=os.environ.copy();env['LD_LIBRARY_PATH']=str(qt/'lib')+':'+str(lib)
            result=subprocess.run(['ldd',str(binary)],capture_output=True,text=True,env=env,check=False)
            for line in result.stdout.splitlines():
                if '=>' not in line:continue
                value=line.split('=>',1)[1].strip().split(' ',1)[0]
                source=pathlib.Path(value)
                if source.is_file() and source.resolve().is_relative_to(qt.resolve()):
                    destination=lib/source.name
                    if not destination.exists():shutil.copy2(source,destination);scan.append(destination)
        (usr/'qt.conf').write_text('[Paths]\nPrefix=.\nLibraries=lib\nPlugins=plugins\nQmlImports=qml\n')
        apprun=appdir/'AppRun';apprun.write_text('#!/bin/sh\nHERE="$(dirname "$(readlink -f "$0")")"\nexport LD_LIBRARY_PATH="$HERE/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexport QT_PLUGIN_PATH="$HERE/usr/plugins"\nexport QML_IMPORT_PATH="$HERE/usr/qml"\nexec "$HERE/usr/GlassStats" "$@"\n');apprun.chmod(0o755)
        (appdir/'glassstats-qt.desktop').write_text('[Desktop Entry]\nType=Application\nName=GlassStats\nExec=GlassStats\nIcon=glassstats-qt\nCategories=System;Monitor;\nTerminal=false\n')
        from PIL import Image
        Image.open(ROOT/'assets/GlassStats.ico').save(appdir/'glassstats-qt.png')
        shutil.copytree(ROOT/'licenses',usr/'licenses',dirs_exist_ok=True)
        run(os.sys.executable,'scripts/prepare_licenses.py','--output',usr/'licenses/Qt')
        for filename in ('LICENSE','README.md','VALIDATION.md','CHANGELOG.md'):shutil.copy2(ROOT/filename,usr/filename)
        tool=ROOT/'.tools/appimagetool-1.9.1-x86_64.AppImage';expected='ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0'
        if not tool.exists():
            with urllib.request.urlopen('https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-x86_64.AppImage',timeout=120) as source,tool.open('wb') as target:shutil.copyfileobj(source,target)
        if hashlib.sha256(tool.read_bytes()).hexdigest()!=expected:raise RuntimeError('Pinned appimagetool checksum mismatch; review/update explicitly')
        tool.chmod(0o755);env=os.environ.copy();env['ARCH']='x86_64';env['APPIMAGE_EXTRACT_AND_RUN']='1'
        image=artifacts/f'GlassStats-Qt-{VERSION}-linux-x64.AppImage';run(tool,appdir,image,env=env)
    (image.parent/(image.name+'.sha256')).write_text(hashlib.sha256(image.read_bytes()).hexdigest()+'  '+image.name+'\n')
    print(image)
if __name__=='__main__':main()
