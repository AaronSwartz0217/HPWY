"""Generate humanoid IK rigs and retarget template idle/run without changing source art."""
import unreal
import json
import pathlib
ROOT = "/Game/HPWH/Locomotion"
tools = unreal.AssetToolsHelpers.get_asset_tools()
def asset(name, cls, factory):
    return unreal.load_asset(ROOT + "/" + name) or tools.create_asset(name, ROOT, cls, factory)
source = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn")
source_rig = asset("IK_Quinn_Source", unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
source_ctrl = unreal.IKRigController.get_controller(source_rig)
source_ctrl.set_skeletal_mesh(source)
source_ctrl.apply_auto_generated_retarget_definition()
unreal.EditorAssetLibrary.save_loaded_asset(source_rig, False)
names = ["铜机械小兵", "铜械战士", "铜械将军", "铜械降神", "竹节人", "竹路人", "竹拳击手", "陶罐头", "陶童子", "陶土行者", "陶土僧人"]
report = []
for name in names:
    if all(unreal.EditorAssetLibrary.does_asset_exist(ROOT + "/Animations/A_" + name + "_" + suffix) for suffix in ["Idle", "Run"]):
        continue
    mesh = unreal.load_asset("/Game/SOURCE/小怪/" + name + "/SK_" + name)
    rig = asset("IK_" + name, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    ctrl = unreal.IKRigController.get_controller(rig)
    ctrl.set_skeletal_mesh(mesh)
    auto = ctrl.apply_auto_generated_retarget_definition()
    component = unreal.new_object(unreal.SkeletalMeshComponent)
    component.set_skeletal_mesh_asset(mesh)
    entry = {"name": name, "auto": auto, "root": str(ctrl.get_retarget_root()), "chains": str(ctrl.get_retarget_chains()), "bones": [str(component.get_bone_name(i)) for i in range(component.get_num_bones())]}
    unreal.EditorAssetLibrary.save_loaded_asset(rig, False)
    report.append(entry)
    pathlib.Path(unreal.Paths.project_saved_dir() + "NPCRetarget.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    assert auto, "Automatic humanoid definition not recognized: " + name
    retarget = asset("RTG_Quinn_" + name, unreal.IKRetargeter, unreal.IKRetargetFactory())
    rc = unreal.IKRetargeterController.get_controller(retarget)
    rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
    rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, rig)
    if rc.get_num_retarget_ops() == 0: rc.add_default_ops()
    rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    rc.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
    unreal.EditorAssetLibrary.save_loaded_asset(retarget, False)
    animations = [unreal.load_asset("/Game/Characters/Mannequins/Animations/Quinn/" + n) for n in ["MF_Idle", "MF_Run_Fwd"]]
    data = [unreal.AssetRegistryHelpers.create_asset_data(a) for a in animations]
    result = unreal.IKRetargetBatchOperation.duplicate_and_retarget(data, source, mesh, retarget, "", "", "NPC_" + name + "_", "", False)
    entry["animations"] = []
    for data in result:
        animation = data.get_asset()
        suffix = "Idle" if "Idle" in animation.get_name() else "Run"
        target = ROOT + "/Animations/A_" + name + "_" + suffix
        assert unreal.EditorAssetLibrary.rename_loaded_asset(animation, target)
        unreal.EditorAssetLibrary.save_loaded_asset(animation, False)
        entry["animations"].append(target)
    entry["retarget_ops"] = rc.get_num_retarget_ops()
    pathlib.Path(unreal.Paths.project_saved_dir() + "NPCRetarget.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("NPC_RETARGET_OK")


import time
quit_at = time.monotonic() + 5.0
def deferred_quit(delta):
    if time.monotonic() >= quit_at:
        unreal.unregister_slate_post_tick_callback(quit_handle)
        unreal.SystemLibrary.quit_editor()
quit_handle = unreal.register_slate_post_tick_callback(deferred_quit)
