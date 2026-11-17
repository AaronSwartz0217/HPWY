"""Render both shader variants with the actual initial player mesh in Unreal."""
import unreal
from pathlib import Path
ROOT='/Game/HPWH/Effects/ChromaticInk'
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
map_path='/Game/HPWH/Maps/L_ChromaticInkPreview'
if unreal.EditorAssetLibrary.does_asset_exist(map_path):
    assert levels.load_level(map_path)
    for actor in actors.get_all_level_actors():
        if not isinstance(actor,unreal.WorldSettings):actors.destroy_actor(actor)
else:
    assert levels.new_level(map_path)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
mesh=unreal.load_asset('/Game/HPWH/Characters/SK_InitialInk_Template')
height=mesh.get_bounds().box_extent.z*2
scale=180./height
for y,variant in [(-125,'Clean'),(125,'Burning')]:
    actor=actors.spawn_actor_from_class(unreal.SkeletalMeshActor,unreal.Vector(0,y,0))
    component=actor.get_component_by_class(unreal.SkeletalMeshComponent)
    component.set_skeletal_mesh_asset(mesh)
    actor.set_actor_scale3d(unreal.Vector(scale,scale,scale))
    actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=-90,roll=0),False)
    component.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    # A neutral reference pose makes the two silhouettes directly comparable.
    for i in range(component.get_num_materials()):component.set_material(i,unreal.load_asset(ROOT+'/MI_InkMatCap_'+variant))
    component.set_overlay_material(unreal.load_asset(ROOT+'/MI_InkOutline_'+variant))
    actor.set_actor_label('ChromaticInk_'+variant)
    label=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(0,y,205))
    text=label.get_component_by_class(unreal.TextRenderComponent)
    text.set_text(variant.upper());text.set_world_size(15.)
    text.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)

floor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-8))
floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
floor.set_actor_scale3d(unreal.Vector(5,8,.1))
light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(200,0,400),unreal.Rotator(pitch=-50,yaw=-30,roll=0))
light.light_component.set_editor_property('intensity',8.)
# The non-emissive contour needs ordinary light, including the camera side.
fill=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(400,0,250),unreal.Rotator(pitch=-20,yaw=180,roll=0))
fill.light_component.set_editor_property('intensity',6.)
fill.light_component.set_editor_property('cast_shadows',False)
levels.save_current_level()

location=unreal.Vector(620,0,135)
rotation=unreal.MathLibrary.find_look_at_rotation(location,unreal.Vector(0,0,105))
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,location,rotation)
component=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
target=unreal.RenderingLibrary.create_render_target2d(world,1440,900,unreal.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property('texture_target',target)
component.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property('fov_angle',49.)
component.set_editor_property('capture_every_frame',False)
component.set_editor_property('capture_on_movement',False)
settings=component.get_editor_property('post_process_settings')
for prop,value in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',0.),('override_auto_exposure_apply_physical_camera_exposure',True),('auto_exposure_apply_physical_camera_exposure',False),('override_bloom_intensity',True),('bloom_intensity',.4)]:settings.set_editor_property(prop,value)
component.set_editor_property('post_process_settings',settings)
# A studio ambient cubemap lights both silhouettes without emission in either material.
settings.set_editor_property('ambient_cubemap',unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
settings.set_editor_property('override_ambient_cubemap_intensity',True)
settings.set_editor_property('ambient_cubemap_intensity',1.)
component.set_editor_property('post_process_settings',settings)
unreal.AutomationLibrary.finish_loading_before_screenshot()
component.capture_scene()
unreal.RenderingLibrary.read_render_target_pixel(world,target,0,0)
output=Path(unreal.Paths.project_saved_dir(),'ChromaticInk');output.mkdir(parents=True,exist_ok=True)
unreal.RenderingLibrary.export_render_target(world,target,str(output),'ChromaticInk_UE.png')
unreal.log('CHROMATIC_CAPTURE_COMPLETE')
