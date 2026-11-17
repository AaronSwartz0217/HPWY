import unreal,json
from pathlib import Path
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for c,v in getattr(unreal,'_cc_probe_lights',[]):c.set_visibility(v)
for cmd in ['ShowFlag.Specular 1','ShowFlag.DirectLighting 1','ShowFlag.GlobalIllumination 1','r.Lumen.DiffuseIndirect.Allow 1','r.Lumen.ScreenProbeGather.ScreenTraces 1']:
 unreal.SystemLibrary.execute_console_command(world,cmd)
rows=[]
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if isinstance(a,unreal.PostProcessVolume):
  row={'actor':a.get_actor_label()};s=a.get_editor_property('settings')
  for p in ['override_lumen_diffuse_color_boost','lumen_diffuse_color_boost','override_lumen_skylight_leaking','lumen_skylight_leaking','override_lumen_scene_lighting_quality','lumen_scene_lighting_quality','override_lumen_scene_detail','lumen_scene_detail','override_lumen_max_trace_distance','lumen_max_trace_distance','override_lumen_final_gather_quality','lumen_final_gather_quality']:
   try:row[p]=str(s.get_editor_property(p))
   except Exception:pass
  rows.append(row)
Path(unreal.Paths.project_saved_dir(),'LiveCourtyardLumenSettings.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
