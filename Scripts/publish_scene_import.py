"""Copy only UE-renamed scene namespaces into the game, refusing existing folders."""
import argparse,json,pathlib,shutil,sys
sys.stdout.reconfigure(encoding='utf-8')
parser=argparse.ArgumentParser();parser.add_argument('folder');args=parser.parse_args()
project=pathlib.Path(__file__).resolve().parents[1];work=project/'Saved/SceneArchiveImport'
report=json.loads((work/(args.folder+'_migration.json')).read_text(encoding='utf-8'))
assert report.get('renameSucceeded') and report.get('saved'),report.keys()
source=work/'Stages'/args.folder/'Content'/'场景资源'/args.folder
destination=project/'Content'/'场景资源'/args.folder
assert source.is_dir() and not destination.exists(),str(destination)
assert destination.resolve().is_relative_to((project/'Content').resolve())
for asset in report['assets']:
    package=asset['new'];relative=package[len(report['prefix'])+1:]
    assert any((source/(relative+ext)).is_file()for ext in ['.uasset','.umap']),package
assert not any(x.is_symlink()for x in source.rglob('*'))
destination.parent.mkdir(parents=True,exist_ok=True)
shutil.copytree(source,destination)
source_files=[x for x in source.rglob('*')if x.is_file()]
assert all((destination/x.relative_to(source)).stat().st_size==x.stat().st_size for x in source_files)
published={'folder':args.folder,'destination':str(destination),'assets':len(report['assets']),'files':len(source_files),'bytes':sum(x.stat().st_size for x in source_files)}
(work/(args.folder+'_published.json')).write_text(json.dumps(published,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(published,ensure_ascii=False))
