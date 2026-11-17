import unreal,json,shutil,datetime
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
backup=root/'Saved/LightingFixBackups'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
backup.mkdir(parents=True,exist_ok=False)
for rel in ['Content/LanTuRebuild/Maps/初始房间/restartlife.umap','Content/LanTuRebuild/Materials/WPQT/模型/杂物模型/摊位布料_001.uasset']:
 src=root/rel;dst=backup/rel;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/LanTuRebuild/Maps/初始房间/restartlife')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
lights=[a for a in actors if a.get_actor_label()=='DirectionalLight' and isinstance(a,unreal.DirectionalLight)]
assert len(lights)==1
light=lights[0].get_component_by_class(unreal.DirectionalLightComponent)
mat=unreal.load_asset('/Game/LanTuRebuild/Materials/WPQT/模型/杂物模型/摊位布料_001')
assert isinstance(mat,unreal.MaterialInstanceConstant)
m=unreal.MaterialEditingLibrary
before={'indirect_lighting_intensity':light.get_editor_property('indirect_lighting_intensity'),'metallic_factor':m.get_material_instance_scalar_parameter_value(mat,'MetallicFactor')}
light.set_editor_property('indirect_lighting_intensity',1.0)
m.set_material_instance_scalar_parameter_value(mat,'MetallicFactor',0.0)
values=list(mat.get_editor_property('scalar_parameter_values'))
parameter=next((v for v in values if str(v.get_editor_property('parameter_info').get_editor_property('name'))=='MetallicFactor'),None)
if parameter is None:
 parameter=unreal.ScalarParameterValue()
 info=unreal.MaterialParameterInfo()
 info.set_editor_property('name','MetallicFactor')
 parameter.set_editor_property('parameter_info',info)
 values.append(parameter)
parameter.set_editor_property('parameter_value',0.0)
mat.set_editor_property('scalar_parameter_values',values)
assert any(str(v.get_editor_property('parameter_info').get_editor_property('name'))=='MetallicFactor' and v.get_editor_property('parameter_value')==0.0 for v in mat.get_editor_property('scalar_parameter_values'))
m.update_material_instance(mat)
assert unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
assert levels.save_current_level()
after={'indirect_lighting_intensity':light.get_editor_property('indirect_lighting_intensity'),'metallic_factor':m.get_material_instance_scalar_parameter_value(mat,'MetallicFactor')}
assert after['indirect_lighting_intensity']==1.0 and after['metallic_factor']==0.0
report={'before':before,'after':after,'backup':str(backup),'saved':True}
(root/'Saved/InitialLevelLightingFix.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
unreal.log('INITIAL_LEVEL_LIGHTING_FIX_SAVED '+json.dumps(report,ensure_ascii=False))
