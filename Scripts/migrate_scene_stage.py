"""Rename scene packages through UE in an isolated project; never rewrite raw uasset bytes."""
import unreal,json,time
from pathlib import Path
project=Path(__file__).resolve().parents[1]
stage=Path(unreal.Paths.project_dir()).resolve()
assert stage.parent==project/'Saved/SceneArchiveImport/Stages'
output=project/'Saved/SceneArchiveImport'
inspection=json.loads((output/(stage.name+'_inspection.json')).read_text(encoding='utf-8'))
prefix='/Game/场景资源/'+stage.name
reg=unreal.AssetRegistryHelpers.get_asset_registry();reg.search_all_assets(synchronous_search=True)
tools=unreal.AssetToolsHelpers.get_asset_tools()
report={'folder':stage.name,'prefix':prefix,'loaded':0,'failedLoads':[],'repairs':[],'assets':[],'warnings':[]}
report_path=output/(stage.name+'_migration.json')
def checkpoint():report_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
rows={x['package']:x for x in inspection['assets']}
assets=[]
# Put worlds last; their referenced content is loaded before package renaming.
for index,row in enumerate(sorted(rows.values(),key=lambda x:(x['class']=='/Script/Engine.World',x['package']))):
    if row['class']=='/Script/CoreUObject.ObjectRedirector':continue
    old=row['package'];new=prefix+old[5:]
    asset=unreal.load_asset(old)
    if not asset:
        asset=unreal.load_asset(new)
    if not asset:
        report['failedLoads'].append(old);continue
    assets.append((old,new,asset));report['loaded']+=1
    if index%50==0:
        checkpoint();unreal.log(f'SCENE_IMPORT_LOAD {stage.name} {index}/{len(rows)}')
packages=list({a.get_outermost().get_name():a.get_outermost()for _,_,a in assets}.values())
fixes={
 '/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny':'/Game/Characters/Mannequins/Meshes/SKM_Manny',
 '/Game/Characters/Heroes/Mannequin/Meshes/SKM_Quinn':'/Game/Characters/Mannequins/Meshes/SKM_Quinn',
 '/Game/Content/Itemy/Thomas/Candle/SM_Smoke':'/Game/AtmosphericCandlesPack/Mesh/SM_Smoke',
 '/Game/Japanese_Village/Levels/L_Showcase2':'/Game/Japanese_Village/Levels/L_Showcase',
}
soft_fixes={}
for old,new in fixes.items():
    if unreal.EditorAssetLibrary.does_asset_exist(new):
        soft_fixes[unreal.SoftObjectPath(old+'.'+old.rsplit('/',1)[1])]=unreal.SoftObjectPath(new+'.'+new.rsplit('/',1)[1])
        report['repairs'].append({'old':old,'new':new,'type':'soft-reference'})
if soft_fixes:tools.rename_referencing_soft_object_paths(packages,soft_fixes)
# These are missing editor preview models in the downloaded packs, not scene geometry.
preview_owners={x['asset']for x in inspection['missingGameDependencies']if x['dependency']in[
 '/Game/Content/Itemy/Thomas/Candle/Candle_eight','/Game/Meshes/Foilage/Grass/Wild_Grass_Flowers_1']}
for old,new,asset in assets:
    if old in preview_owners:
        try:
            asset.set_editor_property('preview_mesh',unreal.SoftObjectPath(''))
            report['repairs'].append({'asset':old,'type':'clear-missing-editor-preview'})
        except Exception as error:report['warnings'].append({'asset':old,'preview':str(error)})
    if old=='/Game/SandStoneCollection/Materials/MTL_SandStoneC01a':
        normal=unreal.load_asset('/Engine/EngineMaterials/DefaultNormal')
        if isinstance(asset,unreal.MaterialInstanceConstant):
            for value in asset.get_editor_property('texture_parameter_values'):
                if not value.get_editor_property('parameter_value'):
                    name=value.get_editor_property('parameter_info').get_editor_property('name')
                    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(asset,name,normal)
                    report['repairs'].append({'asset':old,'parameter':str(name),'type':'missing-normal-neutral-fallback'})
        elif isinstance(asset,unreal.Material):
            node=unreal.MaterialEditingLibrary.get_material_property_input_node(asset,unreal.MaterialProperty.MP_NORMAL)
            if node and isinstance(node,unreal.MaterialExpressionTextureBase) and not node.get_editor_property('texture'):
                node.set_editor_property('texture',normal)
                unreal.MaterialEditingLibrary.recompile_material(asset)
                report['repairs'].append({'asset':old,'type':'missing-normal-neutral-fallback'})
renames=[]
for old,new,asset in assets:
    if asset.get_path_name().split('.')[0]!=new:
        directory,name=new.rsplit('/',1);renames.append(unreal.AssetRenameData(asset,directory,name))
checkpoint();unreal.log(f'SCENE_IMPORT_RENAME_BEGIN {stage.name} {len(renames)}')
report['renameSucceeded']=tools.rename_assets(renames) if renames else True
for old,new,asset in assets:
    report['assets'].append({'old':old,'new':new,'actual':asset.get_path_name(),'class':asset.get_class().get_name()})
    assert asset.get_path_name().split('.')[0]==new,asset.get_path_name()
checkpoint()
new_packages=list({a.get_outermost().get_name():a.get_outermost()for _,_,a in assets}.values())
report['saved']=unreal.EditorLoadingAndSavingUtils.save_packages(new_packages,only_dirty=False)
checkpoint()
assert report['renameSucceeded'] and report['saved'],report_path
unreal.log(f'SCENE_IMPORT_MIGRATION_COMPLETE {stage.name} {len(assets)}')
