import unreal,json
from pathlib import Path
level='/Game/LanTuRebuild/Maps/初始房间/restartlife'
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(level)
r=[]
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 loc=a.get_actor_location();origin,extent=a.get_actor_bounds(False)
 row={'name':a.get_name(),'label':a.get_actor_label(),'class':a.get_class().get_name(),'folder':str(a.get_folder_path()),'loc':[loc.x,loc.y,loc.z],'center':[origin.x,origin.y,origin.z],'extent':[extent.x,extent.y,extent.z],'meshes':[]}
 for c in a.get_components_by_class(unreal.StaticMeshComponent):
  mesh=c.get_editor_property('static_mesh')
  if mesh:row['meshes'].append(mesh.get_path_name())
 r.append(row)
Path(unreal.Paths.project_saved_dir(),'RestartRoomInventory.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
