import unreal,json,math
from pathlib import Path
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/LanTuRebuild/Maps/古风小镇/古风小镇')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
r={'starts':[],'pawn':{}}
gm=world.get_world_settings().get_editor_property('default_game_mode')
if gm:
 r['gamemode']=gm.get_path_name();pawnclass=unreal.get_default_object(gm).get_editor_property('default_pawn_class')
 if pawnclass:
  pawn=unreal.get_default_object(pawnclass);r['pawn']['class']=pawnclass.get_path_name()
  cap=pawn.get_component_by_class(unreal.CapsuleComponent)
  if cap:r['pawn'].update(radius=cap.get_unscaled_capsule_radius(),halfheight=cap.get_unscaled_capsule_half_height())
for start in actors:
 if not isinstance(start,unreal.PlayerStart):continue
 loc=start.get_actor_location();radius=r['pawn'].get('radius',42);height=r['pawn'].get('halfheight',96)
 row={'name':start.get_name(),'location':[loc.x,loc.y,loc.z],'candidates':[],'traces':[]}
 for offset in [0,50,100,200,400]:
  point=loc+unreal.Vector(0,0,offset)
  try:row['traces'].append({'up':offset,'result':str(unreal.SystemLibrary.capsule_trace_multi_by_profile(world,point,point+unreal.Vector(0,0,1),radius,height,'Pawn',False,[start],unreal.DrawDebugTrace.NONE,True))})
  except Exception as e:row['trace_error']=str(e)
 for a in actors:
  if a==start:continue
  for c in a.get_components_by_class(unreal.StaticMeshComponent):
   if str(c.get_collision_enabled()).endswith('NO_COLLISION: 0>'):continue
   origin,extent=c.get_local_bounds();t=c.get_world_transform()
   corners=[t.transform_location(unreal.Vector(x,y,z)) for x in [origin.x,extent.x] for y in [origin.y,extent.y] for z in [origin.z,extent.z]]
   lo=[min(getattr(v,k) for v in corners) for k in ['x','y','z']];hi=[max(getattr(v,k) for v in corners) for k in ['x','y','z']]
   if not (lo[0]-radius<=loc.x<=hi[0]+radius and lo[1]-radius<=loc.y<=hi[1]+radius and lo[2]-height<=loc.z<=hi[2]+height):continue
   mesh=c.get_editor_property('static_mesh');item={'actor':a.get_actor_label(),'component':c.get_name(),'mesh':mesh.get_path_name() if mesh else None,'bounds_min':lo,'bounds_max':hi,'collision':str(c.get_collision_enabled()),'profile':str(c.get_collision_profile_name())}
   try:item['pawn_response']=str(c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN))
   except Exception:pass
   if mesh:
    try:
     body=mesh.get_editor_property('body_setup');item['complexity']=str(body.get_editor_property('collision_trace_flag'));agg=body.get_editor_property('agg_geom')
     item['simple_shapes']={k:len(agg.get_editor_property(k)) for k in ['box_elems','sphere_elems','sphyl_elems','convex_elems']}
    except Exception as e:item['body_error']=str(e)
   row['candidates'].append(item)
 r['starts'].append(row)
Path(unreal.Paths.project_saved_dir(),'TownSpawnCollisionAudit.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('TOWN_SPAWN_COLLISION_AUDIT_COMPLETE')
