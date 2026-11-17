import unreal
root='/Game/HPWH/Locomotion'
mesh=unreal.load_asset('/Game/GameAnimsPack/Demo/Mannequins/Meshes/SKM_Quinn')
bp=unreal.load_asset(root+'/ABP_StateMachineProbe')
if not bp:
 factory=unreal.AnimBlueprintFactory();factory.set_editor_property('parent_class',unreal.CC_CharacterAnimInstance);factory.set_editor_property('target_skeleton',mesh.get_editor_property('skeleton'))
 bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset('ABP_StateMachineProbe',root,unreal.AnimBlueprint,factory)
clips=[unreal.load_asset('/Game/GameAnimsPack/Animations/'+name) for name in ['Anim_Idle_Idle','Anim_Jog_Action','Anim_Jump_Action','Anim_Falling_Reaction','Anim_Land_Reaction','Anim_Death_Reaction']]
assert unreal.CC_AnimationSetupLibrary.build_locomotion_graph(bp,*clips)
unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
unreal.log('STATE_MACHINE_PROBE_PASS')
