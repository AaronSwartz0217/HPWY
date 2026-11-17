import unreal,json
from pathlib import Path
level='/Game/LanTuRebuild/Maps/古风小镇/古风小镇'
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(level)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
def props(o,names):
 out={}
 for n in names:
  try:out[n]=str(o.get_editor_property(n))
  except Exception:pass
 return out
pp=['override_dynamic_global_illumination_method','dynamic_global_illumination_method','override_auto_exposure_method','auto_exposure_method','override_auto_exposure_bias','auto_exposure_bias','override_auto_exposure_min_brightness','auto_exposure_min_brightness','override_auto_exposure_max_brightness','auto_exposure_max_brightness','override_auto_exposure_min_ev100','auto_exposure_min_ev100','override_auto_exposure_max_ev100','auto_exposure_max_ev100','override_indirect_lighting_intensity','indirect_lighting_intensity','override_color_gain','color_gain','override_color_global_gamma','color_global_gamma','override_lumen_skylight_leaking','lumen_skylight_leaking']
r={'map':level,'world':props(world.get_world_settings(),['default_game_mode','force_no_precomputed_lighting','world_to_meters','kill_z']),'lights':[],'postprocess':[],'spawns':[],'skies':[],'cameras':[]}
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
for a in actors:
 info={'label':a.get_actor_label(),'name':a.get_name(),'class':a.get_class().get_name(),'location':str(a.get_actor_location()),'rotation':str(a.get_actor_rotation()),'hidden':a.is_hidden_ed(),'bounds':str(a.get_actor_bounds(False))}
 for c in a.get_components_by_class(unreal.LightComponentBase):
  row=dict(info);row['component']=c.get_class().get_name();row['properties']=props(c,['intensity','intensity_units','light_color','visible','mobility','affects_world','cast_shadows','indirect_lighting_intensity','volumetric_scattering_intensity','attenuation_radius','real_time_capture','source_type','cubemap','lower_hemisphere_is_black','sky_distance_threshold','atmosphere_sun_light','atmosphere_sun_light_index']);r['lights'].append(row)
 if isinstance(a,unreal.PostProcessVolume):
  row=dict(info);row.update(props(a,['enabled','unbound','priority','blend_weight']));row['settings']=props(a.get_editor_property('settings'),pp);r['postprocess'].append(row)
 if isinstance(a,unreal.PlayerStart) or isinstance(a,unreal.Pawn):
  row=dict(info);row.update(props(a,['player_start_tag','auto_possess_player','auto_possess_ai','spawn_collision_handling_method']));row['components']=[]
  for c in a.get_components_by_class(unreal.CapsuleComponent):row['components'].append(props(c,['capsule_half_height','capsule_radius','collision_enabled','collision_profile_name']))
  r['spawns'].append(row)
 for c in a.get_components_by_class(unreal.CameraComponent):
  row=dict(info);row['settings']=props(c.get_editor_property('post_process_settings'),pp);row.update(props(c,['post_process_blend_weight']));r['cameras'].append(row)
 if any(x in a.get_class().get_name().lower() for x in ['sky','atmosphere','fog']):r['skies'].append(info)
Path(unreal.Paths.project_saved_dir(),'TownLightingSpawnAudit.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('TOWN_AUDIT_COMPLETE')
ws=world.get_world_settings();gm=ws.get_editor_property('default_game_mode')
r['game_mode_details']={'class':gm.get_path_name() if gm else None}
if gm:
 cdo=unreal.get_default_object(gm)
 r['game_mode_details'].update(props(cdo,['default_pawn_class','player_controller_class','player_state_class','hud_class','start_players_as_spectators']))
for row in r['spawns']:
 a=next(x for x in actors if x.get_name()==row['name'])
 if not isinstance(a,unreal.PlayerStart):continue
 try:
  start=a.get_actor_location();end=start+unreal.Vector(0,0,0.1)
  result=unreal.SystemLibrary.capsule_trace_multi_by_profile(world,start,end,42.0,96.0,'Pawn',False,[a],unreal.DrawDebugTrace.NONE,True)
  row['pawn_capsule_overlap_probe']=str(result)
 except Exception as e:row['probe_error']=str(e)
Path(unreal.Paths.project_saved_dir(),'TownLightingSpawnAudit.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
