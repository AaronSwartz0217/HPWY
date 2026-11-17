import json,shutil,hashlib
from pathlib import Path
root=Path.cwd(); verify=json.loads((root/'Saved/LanTuStageVerify.json').read_text(encoding='utf-8'));assert verify.get('passed')
source=root/'Saved/LanTuMigrationStage/Content/LanTuRebuild'; target=root/'Content/LanTuRebuild'
assert source.is_dir() and not target.exists(), 'Migration target already exists; inspect instead of overwriting'
shutil.copytree(source,target)
manifest=json.loads((root/'Docs/LanTu资源迁移清单.json').read_text(encoding='utf-8'))
for item in manifest['assets']:
 relative=item['new'].removeprefix('/Game/LanTuRebuild/')
 src=next((source/(relative+ext) for ext in ['.uasset','.umap'] if (source/(relative+ext)).exists()),None)
 assert src
 dst=target/src.relative_to(source)
 assert hashlib.sha256(src.read_bytes()).digest()==hashlib.sha256(dst.read_bytes()).digest(), str(dst)
ini=Path(r'C:\Users\29717\Documents\Unreal Projects\LanTu\Content\Rebuild\restart.ini')
aux=root/'Docs/LanTuMigration/SourceRebuild';aux.mkdir(parents=True,exist_ok=True)
shutil.copy2(ini,aux/'restart.ini')
uproject=root/'CrashCourse.uproject'; project=json.loads(uproject.read_text(encoding='utf-8-sig'))
shutil.copy2(uproject,root/'Saved/CrashCourse.before-lantu.uproject.json')
for name in ['DatasmithContent','Niagara','ControlRig','ACLPlugin','Fab','InterchangeAssets']:
 existing=next((p for p in project['Plugins'] if p['Name']==name),None)
 if existing: existing['Enabled']=True
 else: project['Plugins'].append({'Name':name,'Enabled':True})
uproject.write_text(json.dumps(project,ensure_ascii=False,indent='\t')+'\n',encoding='utf-8')
report={'copiedPackages':len(manifest['assets']),'sha256Verified':True,'root':str(target),'bytes':sum(p.stat().st_size for p in target.rglob('*') if p.is_file())}
(root/'Saved/LanTuMigrationCopy.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False))
