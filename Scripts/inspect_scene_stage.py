"""Inventory references in the isolated extracted scene project without saving assets."""
import unreal,json,collections
from pathlib import Path
project=Path(__file__).resolve().parents[1]
stage=Path(unreal.Paths.project_dir()).resolve()
assert stage.parent==project/'Saved/SceneArchiveImport/Stages',str(stage)
reg=unreal.AssetRegistryHelpers.get_asset_registry()
reg.search_all_assets(synchronous_search=True)
assets=list(reg.get_assets_by_path('/Game',recursive=True))
packages={str(a.package_name)for a in assets}
options=unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True)
missing=[];scripts=set();mounts=set();rows=[]
for asset in assets:
    package=str(asset.package_name);deps=[str(x)for x in reg.get_dependencies(package,options)]
    rows.append({'package':package,'class':str(asset.asset_class_path.package_name)+'.'+str(asset.asset_class_path.asset_name),'dependencies':deps})
    for dep in deps:
        if dep.startswith('/Script/'):scripts.add(dep)
        elif dep.startswith('/Game/'):
            if dep not in packages and not any((stage/'Content'/(dep[6:]+ext)).is_file()for ext in ['.uasset','.umap']):missing.append({'asset':package,'dependency':dep})
        elif dep.startswith('/'):mounts.add(dep.split('/')[1])
report={'folder':stage.name,'assetCount':len(assets),'classes':dict(collections.Counter(x['class']for x in rows)),
 'maps':[x['package']for x in rows if x['class']=='/Script/Engine.World'],'scriptDependencies':sorted(scripts),'pluginMounts':sorted(mounts),'missingGameDependencies':missing,'assets':rows}
out=project/'Saved/SceneArchiveImport'/(stage.name+'_inspection.json')
out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('SCENE_STAGE_INSPECTION_COMPLETE '+json.dumps({k:v for k,v in report.items()if k not in ['assets','classes']},ensure_ascii=False))
