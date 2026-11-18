"""修复材质 Nanite 使用标记，并报告缺失依赖资产的引用者。"""

import unreal

REPORT = r"C:\HPWH\Saved\asset_fix_report.txt"
lines = []

# 1) 给所有导入模型的双面材质补上 Nanite 使用标记并重新保存。
registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path("/Game/HPWH/Assets/Models", recursive=True)
material_count = 0
for asset_data in assets:
    if str(asset_data.asset_class_path.asset_name) != "Material":
        continue
    name = str(asset_data.asset_name)
    if not name.endswith("_TwoSided"):
        continue
    material = asset_data.get_asset()
    if material is None:
        continue
    try:
        material.set_editor_property("used_with_nanite", True)
    except Exception as error:
        lines.append("NANITE FAIL {} {}".format(name, error))
        continue
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    material_count += 1
    lines.append("NANITE OK {}".format(asset_data.package_name))

lines.append("nanite materials updated: {}".format(material_count))

# 2) 报告坏资产的引用者，供决定删除或补占位资产。
broken = [
    "/Game/StarterContent/Props/SM_Rock",
    "/Game/GameAnimsPack/Animations/GAS_Jab",
    "/Game/GameAnimsPack/Animations/MT",
    "/Game/GameAnimsPack/Animations/MT2",
    "/Game/GameAnimsPack/Demo/Mannequins/Materials/Instances/Quinn/MI_Quinn_01",
    "/Game/GameAnimsPack/Demo/Mannequins/Materials/Instances/Quinn/MI_Quinn_02",
    "/Game/LanTuRebuild/Materials/MixedVFX/Materials/MaterialInstances/MI_Trail_01",
]
for path in broken:
    exists = unreal.EditorAssetLibrary.does_asset_exist(path)
    if not exists:
        lines.append("MISSING {}".format(path))
        continue
    referencers = unreal.EditorAssetLibrary.find_package_referencers_for_asset(path, False)
    lines.append("REFERENCERS {} -> {}".format(path, ", ".join(referencers) if referencers else "NONE"))

open(REPORT, "w", encoding="utf-8").write("\n".join(lines))
for line in lines:
    unreal.log("HPWH_FIX " + line)
