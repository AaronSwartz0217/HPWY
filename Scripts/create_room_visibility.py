"""Create the room controller Blueprint and exercise visibility state restoration without saving maps."""
import unreal,json
from pathlib import Path
root='/Game/关卡优化';path=root+'/BP_RoomVisibility'
assert not unreal.EditorAssetLibrary.does_asset_exist(path),'Refusing to overwrite existing resource'
factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.CC_RoomVisibilityController)
bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset('BP_RoomVisibility',root,unreal.Blueprint,factory)
assert bp
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
spawned=[]
def spawn(cls,loc=unreal.Vector(0,0,10000)):
 a=sub.spawn_actor_from_class(cls,loc);assert a;spawned.append(a);return a
checks={}
try:
 controller=spawn(bp.generated_class());visible=spawn(unreal.StaticMeshActor);hidden=spawn(unreal.StaticMeshActor);light=spawn(unreal.PointLight)
 hidden.set_actor_hidden_in_game(True)
 controller.set_editor_property('interior_actors',[visible,hidden]);controller.set_editor_property('room_lights',[light])
 light_component=light.get_component_by_class(unreal.PointLightComponent)
 initial_collision=visible.get_actor_enable_collision() if hasattr(visible,'get_actor_enable_collision') else None
 controller.set_room_active(False)
 checks['interior_hides']=visible.get_editor_property('hidden') and hidden.get_editor_property('hidden')
 checks['light_disables']=not light_component.get_editor_property('visible')
 controller.set_room_active(True)
 checks['visible_restored']=not visible.get_editor_property('hidden')
 checks['originally_hidden_stays_hidden']=hidden.get_editor_property('hidden')
 checks['light_restored']=light_component.get_editor_property('visible')
 light_component.set_visibility(False)
 # A second controller must preserve a light which was disabled before it took ownership.
 other=spawn(bp.generated_class());other.set_editor_property('room_lights',[light]);other.set_room_active(False);other.set_room_active(True)
 checks['originally_disabled_light_stays_disabled']=not light_component.get_editor_property('visible')
 checks['no_controller_tick']=not controller.is_actor_tick_enabled()
 checks['default_trigger_exists']=controller.get_editor_property('trigger_box') is not None
 assert all(checks.values()),checks
finally:
 for a in reversed(spawned):sub.destroy_actor(a)
report={'blueprint':path,'passed':all(checks.values()),'checks':checks,'map_saved':False}
Path(unreal.Paths.project_saved_dir(),'RoomVisibilitySetup.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('ROOM_VISIBILITY_SETUP_PASS')
