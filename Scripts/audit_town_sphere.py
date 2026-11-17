import unreal,json
from pathlib import Path
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/LanTuRebuild/Maps/古风小镇/古风小镇')
r=[]
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if a.get_actor_label()!='Shape_Sphere':continue
 d={'actor':a.get_actor_label(),'scale':str(a.get_actor_scale3d()),'location':str(a.get_actor_location())}
 for c in a.get_components_by_class(unreal.StaticMeshComponent):
  d['materials']=[m.get_path_name() for m in c.get_materials() if m]
  for n in ['cast_shadow','cast_dynamic_shadow','cast_static_shadow','visible','hidden_in_game','use_as_occluder','reverse_culling','body_instance']:
   try:d[n]=str(c.get_editor_property(n))
   except Exception:pass
 r.append(d)
Path(unreal.Paths.project_saved_dir(),'TownSphereAudit.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
