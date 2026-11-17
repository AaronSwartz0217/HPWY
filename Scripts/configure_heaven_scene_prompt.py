from pathlib import Path
import shutil
import unreal
p=Path(unreal.Paths.project_dir());backup=p/'Saved/Backups/HeavenWholeSceneRebuild_20261029'
for name in ['DA_ScanCatalog','DA_Scan_Platform']:
 f=p/f'Content/UI/Scan/{name}.uasset';b=backup/f'Content/UI/Scan/{name}.uasset'
 if f.exists() and not b.exists():b.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,b)
path='/Game/UI/Scan/DA_Scan_HeavenScene';info=unreal.load_asset(path)
if not info:
 factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.CC_ScanDescription)
 info=unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_Scan_HeavenScene','/Game/UI/Scan',unreal.CC_ScanDescription,factory)
info.set_editor_property('display_name','万象天宫 · 天工重构');info.set_editor_property('description','重排整片场景的道路、房屋与广场。按 E 直接切换新布局，无需重组动画。');info.set_editor_property('action_text','随机重组整个场景');info.set_editor_property('max_distance',600.)
unreal.EditorAssetLibrary.save_loaded_asset(info)
catalog=unreal.load_asset('/Game/UI/Scan/DA_ScanCatalog');rules=list(catalog.get_editor_property('rules'))
rules=[r for r in rules if r.get_editor_property('actor_class')!=unreal.WFCMapGenerator.static_class()]
rule=unreal.CC_ScanRule();rule.set_editor_property('actor_class',unreal.WFCMapGenerator.static_class());rule.set_editor_property('info',info);rules.insert(0,rule);catalog.set_editor_property('rules',rules);unreal.EditorAssetLibrary.save_loaded_asset(catalog)
platform=unreal.load_asset('/Game/UI/Scan/DA_Scan_Platform');platform.set_editor_property('description','浮台托举角色升空，离开后下落。万象天宫的 E 用于随机重组整个场景。');platform.set_editor_property('action_text','随机重组整个场景');unreal.EditorAssetLibrary.save_loaded_asset(platform)
unreal.log('HEAVEN_WHOLE_SCENE_PROMPT_CONFIGURED')
