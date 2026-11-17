import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/HPWH/Maps/L_ChromaticInkPreview')
report=[]
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    item={'actor':actor.get_actor_label(),'location':str(actor.get_actor_location()),'rotation':str(actor.get_actor_rotation()),'bounds':str(actor.get_actor_bounds(False))}
    c=actor.get_component_by_class(unreal.SkeletalMeshComponent)
    if c:
        item['component_transform']=str(c.get_world_transform())
        item['head']=str(c.get_socket_location('head'));item['root']=str(c.get_socket_location('root'))
        item['physics']=c.is_simulating_physics()
    report.append(item)
Path(unreal.Paths.project_saved_dir(),'ChromaticPreviewTransforms.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
