"""Preserve the imported coin state machine; configure passive grounded patrols.

Run with UE 5.6 -RenderOffscreen -ExecutePythonScript=<this file>.
Only touches coin assets and the five generated test coins.
"""
import json, time
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
root = '/Game/HPWH/Coins'
art = root + '/Art'
source = '/Game/五铢钱动画/五铢钱动画'
tools = unreal.AssetToolsHelpers.get_asset_tools()
save = lambda obj: unreal.EditorAssetLibrary.save_loaded_asset(obj, False)

# Load the entire original animation graph and its sources BEFORE remapping any
# source packages. This preserves the authored state machine and blend samples.
abp = unreal.load_asset(art + '/ABP_WuzhuCoin')
blend = unreal.load_asset(art + '/BS_WuzhuCoin')
if not abp:
    abp = unreal.load_asset(source + '/AB_五铢钱精怪')
    blend = unreal.load_asset(source + '/NewBlendSpace')
    assert abp and blend, 'Copy original animation blueprint and blend space first'
    names = {'五铢钱':'SK_WuzhuCoin','五铢钱_Skeleton':'SKEL_WuzhuCoin',
             '五铢钱_PhysicsAsset':'PHYS_WuzhuCoin','五铢钱_Anim':'A_WuzhuCoin_Move',
             '五铢钱待机':'A_WuzhuCoin_Idle','材质':'M_WuzhuCoin',
             '材质_001':'M_WuzhuCoin_001','五铢钱_Normal':'T_WuzhuCoin_Normal',
             '五铢钱_-_副本':'T_WuzhuCoin_BaseColor','钱叫声':'S_WuzhuCoin_Pickup'}
    originals = {old: unreal.load_asset(source+'/'+old) for old in names}
    targets = {old: unreal.load_asset(art+'/'+new) for old,new in names.items()}
    assert all(originals.values()) and all(targets.values())
    for old in names:
        if originals[old] != targets[old]:
            unreal.EditorAssetLibrary.consolidate_assets(targets[old], [originals[old]])
    assert tools.rename_assets([unreal.AssetRenameData(abp,art,'ABP_WuzhuCoin'),
                               unreal.AssetRenameData(blend,art,'BS_WuzhuCoin')])
    save(blend)
    unreal.BlueprintEditorLibrary.compile_blueprint(abp)
    save(abp)

mesh = unreal.load_asset(art+'/SK_WuzhuCoin')
sound = unreal.load_asset(art+'/S_WuzhuCoin_Pickup')
assert mesh and sound and abp.generated_class()
bp = unreal.load_asset(root+'/BP_CopperCoin')
assert bp
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
cdo = unreal.get_default_object(bp.generated_class())
assert isinstance(cdo,unreal.Character)
scale = 80. / max(mesh.get_bounds().box_extent.z*2.,1.)
# The imported mesh origin is at its feet. Put it at the capsule bottom,
# rather than at the capsule centre (the cause of the old floating coin).
half = cdo.get_editor_property('capsule_component').get_editor_property('capsule_half_height')
def configure(a):
    a.set_editor_property('pickup_sound',sound)
    a.set_editor_property('copper_amount',5)
    a.set_editor_property('experience_amount',5)
    a.set_editor_property('patrol_speed',100.)
    a.set_editor_property('patrol_radius',600.)
    a.set_editor_property('min_patrol_wait',1.)
    a.set_editor_property('max_patrol_wait',3.)
    a.set_editor_property('auto_possess_ai',unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED)
    comp = a.get_editor_property('coin_mesh')
    comp.set_editor_property('skeletal_mesh_asset',mesh)
    comp.set_relative_scale3d(unreal.Vector(scale,scale,scale))
    comp.set_relative_rotation(unreal.Rotator(pitch=0,yaw=-90,roll=0),False,False)
    comp.set_relative_location(unreal.Vector(0,0,-half),False,False)
    comp.set_animation_mode(unreal.AnimationMode.ANIMATION_BLUEPRINT)
    comp.set_anim_instance_class(abp.generated_class())
    comp.set_editor_property('visibility_based_anim_tick_option',unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
    # AActor -> ACharacter migration: restore attachment on existing templates.
    comp.attach_to_component(a.get_editor_property('capsule_component'),'None',
        unreal.AttachmentRule.KEEP_RELATIVE,unreal.AttachmentRule.KEEP_RELATIVE,
        unreal.AttachmentRule.KEEP_RELATIVE,False)
    sphere = a.get_editor_property('pickup_sphere')
    sphere.attach_to_component(a.get_editor_property('capsule_component'),'None',
        unreal.AttachmentRule.KEEP_RELATIVE,unreal.AttachmentRule.KEEP_RELATIVE,
        unreal.AttachmentRule.KEEP_RELATIVE,False)
configure(cdo)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
save(bp)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/HPWH/Maps/L_FlatTestArena')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
coins = [a for a in actors.get_all_level_actors() if isinstance(a,unreal.CC_CoinPickup)]
records=[]
for a in coins:
    configure(a)
    pos=a.get_actor_location()
    # Ground trace ignores other actors; place capsule above the actual floor.
    hit=unreal.SystemLibrary.line_trace_single(a,unreal.Vector(pos.x,pos.y,pos.z+300),
        unreal.Vector(pos.x,pos.y,pos.z-1000),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        False,[a],unreal.DrawDebugTrace.NONE,True)
    if hit:
        floor=hit.to_tuple()[4]
        a.set_actor_location(unreal.Vector(pos.x,pos.y,floor.z+half+2),False,False)
    records.append({'label':a.get_actor_label(),'location':str(a.get_actor_location()),
                    'root':str(a.get_editor_property('root_component'))})
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
nav_bounds=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.NavMeshBoundsVolume)]
assert nav_bounds, 'The arena needs a NavMeshBoundsVolume'
navs=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.RecastNavMesh)]
if not navs:
    navs=[actors.spawn_actor_from_class(unreal.RecastNavMesh,unreal.Vector())]
    assert navs[0]
for nav in navs:
    nav.set_editor_property('runtime_generation',unreal.RuntimeGenerationType.DYNAMIC)
    nav.set_editor_property('force_rebuild_on_load',True)
unreal.SystemLibrary.execute_console_command(world,'RebuildNavigation')
assert levels.save_current_level()
reg=unreal.AssetRegistryHelpers.get_asset_registry()
deps={}
for name in ('ABP_WuzhuCoin','BS_WuzhuCoin'):
    package=art+'/'+name
    deps[package]=[str(x) for x in reg.get_dependencies(package,unreal.AssetRegistryDependencyOptions(True,True,True,True,True))]
report={'originalStateMachinePreserved':True,'speed':100,'radius':600,'waitSeconds':[1,3],
        'capsuleHalfHeight':half,'meshScale':scale,'meshBounds':str(mesh.get_bounds()),
        'animationClass':abp.generated_class().get_path_name(),'coins':records,'dependencies':deps,
        'navigationBounds':[str(a.get_actor_bounds(False)) for a in nav_bounds],
        'navigationData':[a.get_path_name() for a in navs]}
(project/'Saved/CoinPatrolSetup.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('COIN_PATROL_SETUP_PASS')
quit_at=time.monotonic()+2
def finish(delta):
    if time.monotonic()>quit_at:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
