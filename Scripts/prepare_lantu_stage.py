import json,shutil,hashlib
from pathlib import Path
root=Path.cwd(); audit=json.loads((root/'Saved/LanTuRebuildAudit.json').read_text(encoding='utf-8')); stage=root/'Saved/LanTuMigrationStage'; (stage/'Content').mkdir(parents=True,exist_ok=True)
project={'FileVersion':3,'EngineAssociation':'5.6','Plugins':[{'Name':name,'Enabled':True} for name in ['PythonScriptPlugin','EditorScriptingUtilities','DatasmithContent','Niagara','ControlRig','ACLPlugin','Fab','HDRIBackdrop','DaySequence']]}
(stage/'MigrationStage.uproject').write_text(json.dumps(project,indent=2),encoding='utf-8')
copied=0
for item in audit['assets']:
 source=Path(item['file']); rel=source.relative_to(Path(audit['source'])/'Content'); target=stage/'Content'/rel;target.parent.mkdir(parents=True,exist_ok=True)
 if not target.exists(): shutil.copy2(source,target);copied+=1
 for ext in ['.uexp','.ubulk','.uptnl','.m.ubulk']:
  side=source.with_suffix(ext)
  if side.exists(): shutil.copy2(side,target.with_suffix(ext))
print('Copied',copied,'packages into',str(stage))
print('Meshes:',*[a['package'] for a in audit['assets'] if a['class']=='SkeletalMesh'],sep='\n')
print('Meshes missing from source:',audit['missingSourcePackages'])
