import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/LanTuRebuild/Maps/古风小镇/古风小镇')
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
rows=[]
def vec(v):return [v.x,v.y,v.z]
for actor in sub.get_all_level_actors():
    meshes=actor.get_components_by_class(unreal.StaticMeshComponent)
    paths=[m.get_editor_property('static_mesh').get_path_name() for m in meshes if m.get_editor_property('static_mesh')]
    origin,extent=actor.get_actor_bounds(False)
    if extent.x<100 or extent.y<100:continue
    rows.append(dict(label=actor.get_actor_label(),name=actor.get_name(),className=actor.get_class().get_name(),position=vec(actor.get_actor_location()),rotation=[actor.get_actor_rotation().pitch,actor.get_actor_rotation().yaw,actor.get_actor_rotation().roll],center=vec(origin),extent=vec(extent),meshes=paths,volume=extent.x*extent.y*extent.z))
rows.sort(key=lambda r:r['volume'],reverse=True)
Path(unreal.Paths.project_saved_dir(),'TownBuildingAudit.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('TOWN_BUILDING_AUDIT '+str(len(rows)))
