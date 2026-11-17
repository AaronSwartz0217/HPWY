import unreal,time
asset_tools=unreal.AssetToolsHelpers.get_asset_tools()
task=unreal.AssetImportTask()
task.filename=unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())+'SourceArt/UI/E_Pickup.png'
task.destination_path='/Game/UI/MuralMenu/Textures/Interaction'
task.destination_name='T_Prompt_Pickup'
task.automated=True
task.replace_existing=True
task.save=True
asset_tools.import_asset_tasks([task])
t=unreal.load_asset('/Game/UI/MuralMenu/Textures/Interaction/T_Prompt_Pickup')
assert t
t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
t.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
t.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
t.set_editor_property('never_stream',True)
unreal.EditorAssetLibrary.save_loaded_asset(t,False)
bp=unreal.load_asset('/Game/UI/MuralMenu/BP_MuralHUD')
unreal.get_default_object(bp.generated_class()).set_editor_property('pickup_prompt_texture',t)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
unreal.log('PICKUP_PROMPT_SETUP_PASS')
quit_at=time.monotonic()+3
def done(delta):
 if time.monotonic()>quit_at:
  unreal.unregister_slate_post_tick_callback(handle)
  unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(done)
