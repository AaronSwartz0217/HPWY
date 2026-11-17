import unreal,json,re,hashlib
from pathlib import Path
p=Path(unreal.Paths.project_dir()).resolve();level='/Game/LanTuRebuild/Maps/初始房间/场景优化';folder='/Game/LanTuRebuild/Maps/初始房间/场景优化流送蓝图'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert levels.load_level(level)
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);actors=sub.get_all_level_actors();lib=unreal.EditorAssetLibrary
source=p/'Content/LanTuRebuild/Maps/初始房间/restartlife.umap';source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
def blueprint(name,parent):
 path=folder+'/'+name
 if not lib.does_asset_exist(path):
  f=unreal.BlueprintFactory();f.set_editor_property('parent_class',parent);bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Blueprint,f)
 else:bp=lib.load_asset(path)
 unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert lib.save_loaded_asset(bp,only_if_is_dirty=False)
 return bp.generated_class()
managerclass=blueprint('BP_门控渲染管理器',unreal.CC_DoorVisibilityManager)
visibilityclass=blueprint('BP_门控渲染分组',unreal.CC_RoomVisibilityController)
assert not any(isinstance(a,unreal.CC_DoorVisibilityManager) for a in actors),'Manager already installed; inspect before modifying'
doors=sorted([a for a in actors if isinstance(a,unreal.CC_InteractiveDoubleDoor)],key=lambda a:a.get_actor_location().x);assert len(doors)==3
# The three artist-built interior envelopes. Outer walls, roof and doorway edges are excluded.
regions=[('West',(1030,350,70),(530,950,400),(530,1525,-550,1260,-120,200),doors[0],['P1','P2']),('North',(3350,2900,70),(1150,650,400),(2300,4450,2450,3480,-120,300),doors[1],['P5','P6']),('East',(5780,350,70),(550,950,400),(5270,6270,-550,1260,-120,200),doors[2],['P3','P4'])]
bylabel={a.get_actor_label():a for a in actors};used=set();zones=[];report={'map':level,'rooms':[],'exterior':[],'kept_common':['global lighting','sky','postprocess','building shell','ground','doors','all collision']}
for name,center,half,envelope,door,lights in regions:
 targets=[]
 for a in actors:
  if not isinstance(a,unreal.StaticMeshActor):continue
  if not re.search(r'家具|cha|茶|圆环|^立方体_|瓶|凳|床|榻|桌椅',a.get_actor_label()):continue
  origin,extent=a.get_actor_bounds(False);c=[origin.x,origin.y,origin.z];e=[extent.x,extent.y,extent.z];lo=[envelope[0],envelope[2],envelope[4]];hi=[envelope[1],envelope[3],envelope[5]]
  if not all(c[i]-e[i]>=lo[i] and c[i]+e[i]<=hi[i] for i in range(3)):continue
  assert a.get_name() not in used;used.add(a.get_name());targets.append(a)
 assert targets,'No interior targets for '+name
 group=sub.spawn_actor_from_class(visibilityclass,unreal.Vector(*center));group.set_actor_label('RenderGroup_'+name);group.set_folder_path('关卡优化/门控渲染')
 group.set_editor_property('automatic_player_detection',False);group.set_editor_property('interior_actors',targets);group.set_editor_property('room_lights',[bylabel[x] for x in lights])
 for a in targets:
  for mesh in a.get_components_by_class(unreal.StaticMeshComponent):mesh.set_editor_property('cast_hidden_shadow',False)
 zone=unreal.CC_DoorVisibilityZone();zone.set_editor_property('room_id',name);zone.set_editor_property('door',door);zone.set_editor_property('visibility',group);zone.set_editor_property('center',unreal.Vector(*center));zone.set_editor_property('half_extent',unreal.Vector(*half));zones.append(zone)
 report['rooms'].append({'id':name,'door':door.get_actor_label(),'interior_count':len(targets),'interiors':[a.get_actor_label() for a in targets],'lights':lights,'center':center,'half_extent':half})
exterior=[];fx=[]
for a in actors:
 if a.get_name() in used:continue
 if any(a.get_actor_location().x>=center[0]-half[0] and a.get_actor_location().x<=center[0]+half[0] and a.get_actor_location().y>=center[1]-half[1] and a.get_actor_location().y<=center[1]+half[1] for _,center,half,_,_,_ in regions):continue
 if isinstance(a,unreal.StaticMeshActor):
  mesh=a.get_component_by_class(unreal.StaticMeshComponent).get_editor_property('static_mesh')
  if not mesh:continue
  path=mesh.get_path_name();label=a.get_actor_label()
  if re.search(r'门|墙|屋|地板|地面',label):continue
  if '/杂物模型/' in path or re.search(r'/tree/|草_001|竹子',path,re.I):
   origin,ext=a.get_actor_bounds(False)
   if max(ext.x,ext.y,ext.z)<1500:exterior.append(a)
 if a.get_components_by_class(unreal.NiagaraComponent):fx.append(a)
ex=sub.spawn_actor_from_class(visibilityclass,unreal.Vector(3300,800,0));ex.set_actor_label('RenderGroup_ExteriorDetails');ex.set_folder_path('关卡优化/门控渲染');ex.set_editor_property('automatic_player_detection',False);ex.set_editor_property('interior_actors',exterior);ex.set_editor_property('room_effects',fx)
for a in exterior:
 for mesh in a.get_components_by_class(unreal.StaticMeshComponent):mesh.set_editor_property('cast_hidden_shadow',False)
manager=sub.spawn_actor_from_class(managerclass,unreal.Vector(3300,800,0));manager.set_actor_label('DoorVisibilityManager');manager.set_folder_path('关卡优化/门控渲染');manager.set_editor_property('rooms',zones);manager.set_editor_property('exterior_details',ex)
report['exterior']=[a.get_actor_label() for a in exterior];report['exterior_effects']=[a.get_actor_label() for a in fx];report['manager']=manager.get_path_name();report['restartlife_unchanged']=source_hash==hashlib.sha256(source.read_bytes()).hexdigest()
assert report['restartlife_unchanged'];assert levels.save_current_level()
(p/'Saved/DoorRenderOptimizationSetup.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('DOOR_RENDER_SETUP_PASS '+json.dumps({x['id']:x['interior_count'] for x in report['rooms']}))
