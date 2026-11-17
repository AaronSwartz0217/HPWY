"""Extract only scene Content into an isolated UE project, preserving archive project folders."""
import argparse,json,pathlib,subprocess,sys
sys.stdout.reconfigure(encoding='utf-8')
parser=argparse.ArgumentParser();parser.add_argument('archive');args=parser.parse_args()
project=pathlib.Path(__file__).resolve().parents[1]
work=project/'Saved/SceneArchiveImport'
inventory=json.loads((work/'archive_inventory.json').read_text(encoding='utf-8'))
record=next(x for x in inventory if x['archive']==args.archive)
archive=pathlib.Path('C:/Users/29717/Desktop')/(args.archive+'.rar')
entries=json.loads((work/(args.archive+'_entries.json')).read_text(encoding='utf-8'))
assert archive.is_file()
assert len(record['roots'])==1
root=record['roots'][0]
for entry in entries:
    path=pathlib.PurePosixPath(entry.replace('\\','/'))
    assert not path.is_absolute() and '..'not in path.parts and ':'not in entry,entry
    assert not entry.endswith('/init_unreal.py'),entry
stage=work/'Stages'/root
assert not stage.exists(),f'Refusing to overwrite stage: {stage}'
stage.parent.mkdir(parents=True,exist_ok=True)
run=subprocess.run(['tar','-xf',str(archive),'-C',str(stage.parent),'--no-same-owner','--no-same-permissions',root+'/Content'],capture_output=True)
if run.returncode:
    print(run.stderr.decode('gb18030',errors='replace'));sys.exit(run.returncode)
assert (stage/'Content').is_dir()
plugins=['PythonScriptPlugin','EditorScriptingUtilities','DatasmithContent','HDRIBackdrop']
(stage/'SceneImportStage.uproject').write_text(json.dumps({'FileVersion':3,'EngineAssociation':'5.6','Plugins':[{'Name':name,'Enabled':True}for name in plugins]},indent=2),encoding='utf-8')
(stage/'Config').mkdir(exist_ok=True)
(stage/'Config/DefaultEngine.ini').write_text('[/Script/Engine.RendererSettings]\nr.DynamicGlobalIlluminationMethod=1\nr.ReflectionMethod=1\nr.SkinCache.CompileShaders=True\nr.AllowStaticLighting=False\n',encoding='utf-8')
files=list((stage/'Content').rglob('*'))
report={'archive':str(archive),'originalFolder':root,'stage':str(stage),'projectFile':str(stage/'SceneImportStage.uproject'),'files':sum(x.is_file()for x in files),'bytes':sum(x.stat().st_size for x in files if x.is_file())}
(work/(args.archive+'_extraction.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False))
