"""Inspect final game namespaces and open each imported primary map without playing."""
import unreal,json
from pathlib import Path
project=Path(__file__).resolve().parents[1]
assert Path(unreal.Paths.project_dir()).resolve()==project
work=project/'Saved/SceneArchiveImport'
reg=unreal.AssetRegistryHelpers.get_asset_registry();reg.search_all_assets(synchronous_search=True)
options=unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mains={'China_townnight':'Japanese_Village/Levels/L_Showcase','乌云庙宇':'NewMap','黄山':'zuopin_1/zuoping_demo'}
result=[]
for folder,main in mains.items():
    migration=json.loads((work/(folder+'_migration.json')).read_text(encoding='utf-8'))
    prefix=migration['prefix'];rows=reg.get_assets_by_path(prefix,recursive=True)
    packages={str(x.package_name)for x in rows}
    missing=[];old_refs=[]
    old_packages={x['old']for x in migration['assets']}
    for package in packages:
        for dep in reg.get_dependencies(package,options):
            dep=str(dep)
            if dep.startswith('/Game/'):
                path=project/'Content'/dep[6:]
                exists=any(path.with_name(path.name+ext).is_file()for ext in ['.uasset','.umap'])
                if not exists:missing.append({'asset':package,'dependency':dep})
                if dep in old_packages:old_refs.append({'asset':package,'dependency':dep})
    primary=prefix+'/'+main
    loaded=levels.load_level(primary)
    row={'folder':folder,'prefix':prefix,'assetCount':len(rows),'expected':len(migration['assets']),'primaryMap':primary,'mapLoaded':loaded,
         'actors':len(actors.get_all_level_actors())if loaded else 0,'missingGameReferences':missing,'unremappedOriginalReferences':old_refs}
    result.append(row)
    (work/'final_verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.log('IMPORTED_SCENE_VERIFIED '+json.dumps({k:v for k,v in row.items()if k not in ['missingGameReferences','unremappedOriginalReferences']},ensure_ascii=False))
assert all(x['mapLoaded']and x['assetCount']>=x['expected']and not x['unremappedOriginalReferences']for x in result)
unreal.log('THREE_SCENES_IMPORT_VERIFIED')
