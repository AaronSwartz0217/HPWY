"""强制把五铢钱贴图复制到旧引用路径，替换遗留 redirector。"""

import os

import unreal

CONTENT = unreal.Paths.project_content_dir()
OLD_DIR = "/Game/ProgressionDemo/Coins/Art"
NEW_DIR = "/Game/HPWH/Coins/Art"
NAMES = ("SKEL_WuzhuCoin", "T_WuzhuCoin_BaseColor", "T_WuzhuCoin_Normal", "BS_WuzhuCoin")

tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {}

for name in NAMES:
    old_path = "{}/{}".format(OLD_DIR, name)
    new_path = "{}/{}".format(NEW_DIR, name)
    disk_path = os.path.join(CONTENT, "ProgressionDemo", "Coins", "Art", name + ".uasset")
    report[name + "_before"] = os.path.exists(disk_path)
    if not os.path.exists(disk_path):
        if unreal.EditorAssetLibrary.does_asset_exist(old_path):
            unreal.EditorAssetLibrary.delete_asset(old_path)
        duplicate = tools.duplicate_asset(
            asset_name=name,
            package_path=OLD_DIR,
            original_object=unreal.load_asset(new_path))
        report[name + "_duplicate"] = str(duplicate)
        unreal.EditorAssetLibrary.save_asset(old_path)
    report[name + "_after"] = os.path.exists(disk_path)

report["legacy_dir"] = os.listdir(os.path.join(CONTENT, "ProgressionDemo", "Coins", "Art")) if os.path.isdir(
    os.path.join(CONTENT, "ProgressionDemo", "Coins", "Art")) else "missing"

import json  # noqa: E402

with open(unreal.Paths.project_saved_dir() + "CoinTextureFix.json", "w", encoding="utf-8") as handle:
    json.dump(report, handle, ensure_ascii=False, indent=2)

unreal.SystemLibrary.quit_editor()
