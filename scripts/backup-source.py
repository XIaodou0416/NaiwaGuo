"""Create an immutable private source snapshot for every build/version."""
from pathlib import Path
import hashlib,json,re,subprocess,zipfile
root=Path(__file__).resolve().parent.parent
version=next(line.split(':',1)[1].strip() for line in (root/'control').read_text().splitlines() if line.startswith('Version:'))
assert re.fullmatch(r'[0-9]+(?:\.[0-9]+)+',version)
try:
 git_root=subprocess.check_output(['git','rev-parse','--show-toplevel'],cwd=root,stderr=subprocess.DEVNULL,text=True).strip()
 checkout=Path(git_root).resolve()==root
except (FileNotFoundError,subprocess.CalledProcessError):
 checkout=False
if checkout:
 paths=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=root).decode().split('\0')
 commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
 dirty=bool(subprocess.check_output(['git','status','--porcelain'],cwd=root,text=True).strip())
else:
 # Release source ZIPs have no Git metadata. Select only project source roots,
 # never generated build/dist output, caches, credentials or a parent checkout.
 paths=[p.name for p in root.iterdir() if p.is_file() and
        (p.suffix=='.md' or p.name in {'.gitignore','LICENSE','control','NaiwaGuo.plist','SOURCE-MANIFEST.json'})]
 for directory in ('src','scripts','tests','assets','docs','third_party'):
  paths.extend(p.relative_to(root).as_posix() for p in (root/directory).rglob('*')
               if p.is_file() and '__pycache__' not in p.parts and p.name!='.DS_Store'
               and not p.name.startswith('.env') and p.suffix not in {'.pyc','.log'})
 exported=root/'SOURCE-MANIFEST.json'
 commit=json.loads(exported.read_text()).get('gitCommit') if exported.exists() else None
 if commit is not None and not re.fullmatch(r'[a-f0-9]{40}',commit):
  raise ValueError('Invalid source manifest Git commit')
 dirty=None
files=sorted({p for p in paths if p and (root/p).is_file()})
for name in files:
 p=root/name
 assert not p.is_symlink(),name
 assert p.suffix.lower() not in {'.ipa','.p12','.p8','.mobileprovision','.dylib','.deb','.zip'},name
 if p.suffix.lower() not in {'.png','.jpg','.jpeg'}:
  assert not re.search(rb'ghp_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY',p.read_bytes()),name
hashes={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in files}
snapshot=hashlib.sha256(json.dumps(hashes,sort_keys=True).encode()).hexdigest()
out=root/'dist/private-backups'/version;out.mkdir(parents=True,exist_ok=True)
label=('working' if dirty else 'committed') if checkout else 'exported'
archive=out/f'NaiwaGuo-{version}-source-{commit[:8] if commit else "unknown"}-{snapshot[:12]}-{label}.zip'
manifest={'version':version,'gitCommit':commit,'workingTreeDirty':dirty,'sourceSnapshotSHA256':snapshot,'files':hashes,'privateBackup':True}
if not archive.exists():
 with zipfile.ZipFile(archive,'x',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for name in files:z.write(root/name,str(Path(f'NaiwaGuo-{version}-source')/name))
  z.writestr('BACKUP-MANIFEST.json',json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
with zipfile.ZipFile(archive) as z:
 assert z.testzip() is None
 for name,digest in hashes.items():assert hashlib.sha256(z.read(str(Path(f'NaiwaGuo-{version}-source')/name))).hexdigest()==digest
archive.with_suffix('.sha256').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n')
print(json.dumps({'privateSourceBackup':str(archive.relative_to(root)),'sourceFiles':len(files),'snapshot':snapshot,'dirty':dirty},ensure_ascii=False))
