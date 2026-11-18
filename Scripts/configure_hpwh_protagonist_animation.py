import unreal as u
L=u.EditorAssetLibrary
T=u.AssetToolsHelpers.get_asset_tools()
root='/Game/HPWH/Locomotion/Protagonist'
base='/Game/HPWH/Locomotion/'
def asset(name, cls, factory):
    return u.load_asset(root+'/'+name) or T.create_asset(name, root, cls, factory)
def save(a):
    assert a and L.save_loaded_asset(a,False),str(a)
mesh=u.load_asset('/Game/zichan/model/主角/SkeletalMeshes/主角')
source_rig=u.load_asset('/Game/Characters/Mannequins/Rigs/IK_Mannequin')
source=u.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny')
assert mesh and source
target=asset('IK_Protagonist',u.IKRigDefinition,u.IKRigDefinitionFactory())
rc=u.IKRigController.get_controller(target)
rc.set_skeletal_mesh(mesh)
assert rc.apply_auto_generated_retarget_definition()
save(target)
rtg=asset('RTG_Manny_Protagonist',u.IKRetargeter,u.IKRetargetFactory())
ctrl=u.IKRetargeterController.get_controller(rtg)
ctrl.set_ik_rig(u.RetargetSourceOrTarget.SOURCE,source_rig)
ctrl.set_ik_rig(u.RetargetSourceOrTarget.TARGET,target)
if ctrl.get_num_retarget_ops()==0: ctrl.add_default_ops()
ctrl.auto_map_chains(u.AutoMapChainType.FUZZY,True)
ctrl.auto_align_all_bones(u.RetargetSourceOrTarget.TARGET)
save(rtg)
kinds=['Idle','Run','Jump','Fall','Land','Death']
for kind in kinds:
    dest=root+'/A_Protagonist_'+kind
    if L.does_asset_exist(dest): continue
    suffix={'Idle':'Idle','Run':'Run_Fwd','Jump':'Jump','Fall':'Fall_Loop','Land':'Land','Death':'Idle'}[kind]
    original=u.load_asset('/Game/Characters/Mannequins/Animations/Manny/MM_'+suffix)
    assert original
    results=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(original)],source,mesh,rtg,'','','HPWH_Protagonist'+kind+'_','',False)
    assert len(results)==1,kind
    assert L.rename_loaded_asset(results[0].get_asset(),dest)
clips=[u.load_asset(root+'/A_Protagonist_'+k) for k in kinds]
skeleton=mesh.get_editor_property('skeleton')
assert all(c and c.get_editor_property('skeleton')==skeleton and c.get_play_length()>0 for c in clips)
factory=u.AnimBlueprintFactory()
factory.set_editor_property('parent_class',u.CC_CharacterAnimInstance)
factory.set_editor_property('target_skeleton',skeleton)
bp=asset('ABP_Protagonist',u.AnimBlueprint,factory)
assert u.CC_AnimationSetupLibrary.build_locomotion_graph(bp,*clips)
save(bp)
u.log('HPWH_PROTAGONIST_ANIMATION_SAVED')
