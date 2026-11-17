import unreal
from pathlib import Path
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/LanTuRebuild/Maps/初始房间/restartlife')
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
out=Path(unreal.Paths.project_saved_dir(),'RoomVisibilityTrial');out.mkdir(exist_ok=True)
for name,position,look in [('WestRoom',(2000,350,50),(800,350,50)),('EastRoom',(4700,350,50),(5900,350,50)),('NorthRoom',(3400,2100,100),(3400,3400,100))]:
 location=unreal.Vector(*position);rot=unreal.MathLibrary.find_look_at_rotation(location,unreal.Vector(*look))
 a=sub.spawn_actor_from_class(unreal.SceneCapture2D,location,rot);c=a.get_component_by_class(unreal.SceneCaptureComponent2D)
 target=unreal.RenderingLibrary.create_render_target2d(world,1100,700,unreal.TextureRenderTargetFormat.RTF_RGBA8)
 c.set_editor_property('texture_target',target);c.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR);c.set_editor_property('capture_every_frame',False);c.set_editor_property('capture_on_movement',False);c.set_editor_property('fov_angle',85.)
 settings=c.get_editor_property('post_process_settings')
 for k,v in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_apply_physical_camera_exposure',True),('auto_exposure_apply_physical_camera_exposure',False),('override_auto_exposure_bias',True),('auto_exposure_bias',1.)]:settings.set_editor_property(k,v)
 c.set_editor_property('post_process_settings',settings)
 c.capture_scene();unreal.RenderingLibrary.read_render_target_pixel(world,target,0,0)
 unreal.RenderingLibrary.export_render_target(world,target,str(out),name+'.png');sub.destroy_actor(a)

