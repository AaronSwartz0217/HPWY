"""Render a reproducible still of the migrated mesh/material in UE, without image synthesis."""
from pathlib import Path
import unreal

project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
out=project/'Saved/UnityVFXProbe';out.mkdir(parents=True,exist_ok=True)
root='/Game/UnityVFXProbe'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(root+'/Maps/L_Shockwave_Probe')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
M=unreal.MaterialEditingLibrary
source=unreal.load_asset(root+'/Materials/M_Shockwave_Prototype')
tools=unreal.AssetToolsHelpers.get_asset_tools()
capture_mat=unreal.load_asset(root+'/Materials/M_Shockwave_Snapshot')
if capture_mat:
    unreal.EditorAssetLibrary.delete_asset(root+'/Materials/M_Shockwave_Snapshot')
capture_mat=tools.duplicate_asset('M_Shockwave_Snapshot',root+'/Materials',source)
pending=[M.get_material_property_input_node(capture_mat,p) for p in [unreal.MaterialProperty.MP_EMISSIVE_COLOR,unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET]]
seen=set();customs=[]
while pending:
    node=pending.pop()
    if node is None or node.get_path_name() in seen:continue
    seen.add(node.get_path_name())
    if isinstance(node,unreal.MaterialExpressionCustom):customs.append(node)
    pending.extend(M.get_inputs_for_material_expression(capture_mat,node))
assert len(customs)==2
frozen=M.create_material_expression(capture_mat,unreal.MaterialExpressionScalarParameter)
frozen.set_editor_property('parameter_name','SnapshotTime')
frozen.set_editor_property('default_value',.35)
for node in customs:assert M.connect_material_expressions(frozen,'',node,'Clock')
M.recompile_material(capture_mat)
unreal.EditorAssetLibrary.save_loaded_asset(capture_mat)

bp=unreal.load_asset(root+'/Blueprints/BP_Shockwave_Prototype')
for actor in actors.get_all_level_actors():
    if actor.get_class()==bp.generated_class():
        mesh=actor.static_mesh_component.get_editor_property('static_mesh')
        extent=mesh.get_bounds().box_extent
        scale=400/max(extent.x,extent.y,extent.z,1)
        actor.set_actor_scale3d(unreal.Vector(scale,scale,scale))
        actor.set_actor_rotation(unreal.Rotator(0,0,90),False)
        actor.set_actor_location(unreal.Vector(0,0,50),False,False)
        actor.static_mesh_component.set_material(0,capture_mat)
    elif isinstance(actor,unreal.DirectionalLight):
        actor.light_component.set_editor_property('intensity',3)

location=unreal.Vector(650,-850,650)
rotation=unreal.MathLibrary.find_look_at_rotation(location,unreal.Vector(0,0,40))
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,location,rotation)
component=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
target=unreal.RenderingLibrary.create_render_target2d(world,1440,900,unreal.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property('texture_target',target)
component.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property('fov_angle',50)
component.set_editor_property('capture_every_frame',False)
component.set_editor_property('capture_on_movement',False)
unreal.AutomationLibrary.finish_loading_before_screenshot()
component.capture_scene()
unreal.RenderingLibrary.read_render_target_pixel(world,target,0,0)
unreal.RenderingLibrary.export_render_target(world,target,str(out),'Shockwave_UE_Render.png')
unreal.log('UNITY_VFX_CAPTURE_PASS '+str(out/'Shockwave_UE_Render.png'))
unreal.SystemLibrary.quit_editor()
