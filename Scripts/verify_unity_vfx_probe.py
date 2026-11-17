"""Reload saved probe assets, verify material bindings and normalize the preview scale."""
from pathlib import Path
import json,time
import unreal

project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
root='/Game/UnityVFXProbe'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(root+'/Maps/L_Shockwave_Probe')
mat=unreal.load_asset(root+'/Materials/M_Shockwave_Prototype');assert mat
bp=unreal.load_asset(root+'/Blueprints/BP_Shockwave_Prototype');assert bp and bp.generated_class()
component=unreal.get_default_object(bp.generated_class()).get_editor_property('static_mesh_component')
assert component.get_editor_property('static_mesh')
assert component.get_material(0)==mat
stats=unreal.MaterialEditingLibrary.get_statistics(mat)
unreal.log('UNITY_VFX_RELOAD_PASS '+str(stats))
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
extent=component.get_editor_property('static_mesh').get_bounds().box_extent
scale=300/max(extent.x,extent.y,extent.z,1)
unreal.log('UNITY_VFX_MESH_EXTENT '+str(extent)+' preview scale '+str(scale))
for actor in actors.get_all_level_actors():
    if actor.get_class()==bp.generated_class():actor.set_actor_scale3d(unreal.Vector(scale,scale,scale))
location=unreal.Vector(700,-900,600)
rotation=unreal.MathLibrary.find_look_at_rotation(location,unreal.Vector(0,0,100))
camera=actors.spawn_actor_from_class(unreal.CameraActor,location,rotation)
camera.camera_component.set_editor_property('field_of_view',65)
unreal.EditorLevelLibrary.set_level_viewport_camera_info(location,rotation)
levels.save_current_level()
result_path=project/'Saved/UnityVFXProbe/result.json'
result=json.loads(result_path.read_text(encoding='utf-8'))
result['reload_verified']=True
result['preview_scale']=scale
result['visual_parity_verified']=False
result_path.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
