import unreal,json,shutil,datetime
from pathlib import Path
root=Path(unreal.Paths.project_dir())
asset='/Game/场景资源/China_townnight/AmbientFx/Blueprints/BP_Ambient_Game_Fx'
p=root/'Content/场景资源/China_townnight/AmbientFx/Blueprints/BP_Ambient_Game_Fx.uasset'
backup=root/'Saved/AmbientLeafBackups'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
backup.mkdir(parents=True,exist_ok=True);shutil.copy2(p,backup/p.name)
bp=unreal.load_asset(asset);cls=unreal.EditorAssetLibrary.load_blueprint_class(asset);cdo=unreal.get_default_object(cls)
old_enum=cdo.get_editor_property('Ambient_Type')
# Resolve the imported enum by its display name rather than assuming its numeric order.
enum_type=type(old_enum)
choices=[getattr(enum_type,n) for n in dir(enum_type) if 'JUNGLE' in n.upper()]
assert choices, str(dir(enum_type))
settings={'Ambient_Type':choices[0],'Air_Leaf_SpawnRate':30.0,'Air_Leaf_Lifetime':2.0,'Air_Leaf_Gravity':1.0,'Air_Leaf_WindStrength':1.5,'Air_Leaf_StormStrength':0.0,'Ground_Leaf_SpawnRate':8.0,'Ground_Leaf_Lifetime':4.0,'Ground_Leaf_WindStrength':0.2,'Ground_Leaf_StormStrength':0.0,'Scan Visualization?':False}
r={'backup':str(backup),'before':{},'after':{}}
for k,v in settings.items():
 r['before'][k]=str(cdo.get_editor_property(k));cdo.set_editor_property(k,v)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
cdo=unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(asset))
for k,v in settings.items():
 actual=cdo.get_editor_property(k);assert actual==v,(k,actual,v)
 r['after'][k]=str(actual)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
r['saved']=True
(root/'Saved/AmbientLeafAdjustment.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('AMBIENT_LEAF_ADJUSTMENT_SAVED')
