import unreal,json
from pathlib import Path
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for c,v in getattr(unreal,'_cc_probe_lights',[]):c.set_visibility(v)
for cmd in ['ShowFlag.Specular 1','ShowFlag.DirectLighting 1','ShowFlag.GlobalIllumination 1','r.Lumen.DiffuseIndirect.Allow 0']:
 unreal.SystemLibrary.execute_console_command(world,cmd)
rows=[]
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if isinstance(a,unreal.DirectionalLight):rows.append({'actor':a.get_actor_label(),'rotation':str(a.get_actor_rotation())})
w=world.get_world_settings()
for p in ['force_no_precomputed_lighting','dynamic_indirect_lighting_method']:
 try:rows.append({p:str(w.get_editor_property(p))})
 except Exception:pass
Path(unreal.Paths.project_saved_dir(),'LiveCourtyardGIProbe.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
