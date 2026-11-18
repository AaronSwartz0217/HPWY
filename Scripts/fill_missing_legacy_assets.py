"""为缺失的旧内容依赖补最小占位资产，消除加载报错。"""

import unreal

PNG = r"C:\Users\29717\AppData\Local\Temp\hpwh_story_assets\gray.png"
REPORT = r"C:\HPWH\Saved\placeholder_report.txt"
lines = []

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

texture_targets = [
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_01_ASAOMASK_MSK"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_01_CCRCCPlastic_MSK"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_01ID_BN"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_01ID_D"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_01ID_MSR_MSK"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_01ID_N"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_01ID_Tan"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_02_ASAOMASK_MSK"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_02_CCRCCPlastic_MSK"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_02ID_BN"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_02ID_D"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_02ID_MSR_MSK"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_02ID_N"),
    ("/Game/GameAnimsPack/Demo/Mannequins/Textures/Quinn", "T_Quinn_02ID_Tan"),
    ("/Game/LanTuRebuild/Textures/MixedVFX/Textures", "T_Mask_01"),
    ("/Game/LanTuRebuild/Textures/MixedVFX/Textures", "T_Trail_01"),
]

tasks = []
for folder, name in texture_targets:
    path = folder + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        lines.append("TEX EXISTS " + path)
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", PNG)
    task.set_editor_property("destination_path", folder)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)

if tasks:
    asset_tools.import_asset_tasks(tasks)
    lines.append("TEX IMPORTED {}".format(len(tasks)))


def ensure_material(path, color, translucent=False):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        lines.append("MAT EXISTS " + path)
        return
    folder, name = path.rsplit("/", 1)
    material = asset_tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if material is None:
        lines.append("MAT FAIL " + path)
        return
    if translucent:
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    constant = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -320, 0)
    constant.set_editor_property("constant", color)
    unreal.MaterialEditingLibrary.connect_material_property(
        constant, "", unreal.MaterialProperty.MP_BASE_COLOR)
    if translucent:
        opacity = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant, -320, 180)
        opacity.set_editor_property("r", 0.55)
        unreal.MaterialEditingLibrary.connect_material_property(
            opacity, "", unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    lines.append("MAT OK " + path)


ensure_material("/Game/StarterContent/Props/Materials/M_Rock", unreal.LinearColor(0.22, 0.22, 0.24, 1.0))
ensure_material("/Game/GameAnimsPack/Demo/Mannequins/Materials/M_Mannequin", unreal.LinearColor(0.62, 0.60, 0.58, 1.0))
ensure_material("/Game/LanTuRebuild/Materials/MixedVFX/Materials/M_TrailMaster",
    unreal.LinearColor(0.55, 0.80, 1.0, 1.0), translucent=True)

# 动画占位：沿用混合空间自身的骨架，保证样本有效。
blend_space = unreal.EditorAssetLibrary.load_asset("/Game/GameAnimsPack/Animations/GAS_Jab")
skeleton = blend_space.get_editor_property("skeleton") if blend_space else None
lines.append("SKELETON {}".format(skeleton.get_path_name() if skeleton else "NONE"))

for name in ("Anim_Jog_Action", "Anim_CrossJab_Action", "Anim_Uppercut_Action"):
    path = "/Game/GameAnimsPack/Animations/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        lines.append("ANIM EXISTS " + path)
        continue
    try:
        factory = unreal.AnimSequenceFactory()
        factory.set_editor_property("target_skeleton", skeleton)
        anim = asset_tools.create_asset(name, "/Game/GameAnimsPack/Animations", unreal.AnimSequence, factory)
        if anim:
            unreal.EditorAssetLibrary.save_loaded_asset(anim)
            lines.append("ANIM OK " + path)
        else:
            lines.append("ANIM FAIL " + path)
    except Exception as error:
        lines.append("ANIM ERROR {} {}".format(path, error))

notify_path = "/Game/THAT/Notifies/AN_SendEventToActor"
if not unreal.EditorAssetLibrary.does_asset_exist(notify_path):
    try:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.AnimNotify)
        bp = asset_tools.create_asset("AN_SendEventToActor", "/Game/THAT/Notifies", unreal.Blueprint, factory)
        if bp:
            unreal.EditorAssetLibrary.save_loaded_asset(bp)
            lines.append("NOTIFY OK " + notify_path)
        else:
            lines.append("NOTIFY FAIL " + notify_path)
    except Exception as error:
        lines.append("NOTIFY ERROR {}".format(error))
else:
    lines.append("NOTIFY EXISTS " + notify_path)

open(REPORT, "w", encoding="utf-8").write("\n".join(lines))
for line in lines:
    unreal.log("HPWH_FILL " + line)
