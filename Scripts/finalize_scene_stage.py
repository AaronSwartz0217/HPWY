"""Resave final destination packages in a fresh editor process after an asset move."""
import unreal,json
from pathlib import Path
project=Path(__file__).resolve().parents[1];stage=Path(unreal.Paths.project_dir()).resolve()
assert stage.parent==project/'Saved/SceneArchiveImport/Stages'
path=project/'Saved/SceneArchiveImport'/(stage.name+'_migration.json')
report=json.loads(path.read_text(encoding='utf-8'));assert report['renameSucceeded']
assets=[]
for index,row in enumerate(report['assets']):
    asset=unreal.load_asset(row['new']);assert asset,row['new'];assets.append(asset)
    if index%100==0:unreal.log(f'SCENE_IMPORT_FINALIZE {stage.name} {index}/{len(report["assets"])}')
packages=list({asset.get_outermost().get_name():asset.get_outermost()for asset in assets}.values())
soft_fixes={}
for row in report['assets']:
    old=row['old']+'.'+row['old'].rsplit('/',1)[1];new=row['new']+'.'+row['new'].rsplit('/',1)[1]
    soft_fixes[unreal.SoftObjectPath(old)]=unreal.SoftObjectPath(new)
    if 'Blueprint' in row['class']:soft_fixes[unreal.SoftObjectPath(old+'_C')]=unreal.SoftObjectPath(new+'_C')
unreal.AssetToolsHelpers.get_asset_tools().rename_referencing_soft_object_paths(packages,soft_fixes)
report['freshDestinationSave']=unreal.EditorLoadingAndSavingUtils.save_packages(packages,only_dirty=False)
report['saved']=report['freshDestinationSave']
path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
assert report['saved']
unreal.log('SCENE_IMPORT_FINALIZE_COMPLETE '+stage.name)
