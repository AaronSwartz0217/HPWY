import os
import unreal

SOURCE_DIR = r"C:\Users\29717\Desktop\幻魄物语\模型资产"
DEST_PATH = "/Game/HPWH/Assets/Models"

unreal.log("HPWH_IMPORT start source_exists={}".format(os.path.isdir(SOURCE_DIR)))
files = [f for f in os.listdir(SOURCE_DIR) if f.lower().endswith(".glb")]
unreal.log("HPWH_IMPORT glb_count={}".format(len(files)))

NAME_MAP = {
    "一本书": "one_book",
    "三本书": "three_books",
    "书堆": "book_pile",
    "纸团": "paper_ball",
    "积木": "block",
    "台灯": "desk_lamp",
    "八卦镜": "bagua_mirror",
    "四合院模型": "courtyard_model",
    "笔记本电脑": "laptop",
    "主角": "protagonist",
    "徘徊者": "crawler",
    "摄魂蜘蛛": "soul_spider",
}

tasks = []
for filename in sorted(files):
    stem = os.path.splitext(filename)[0]
    dest_name = NAME_MAP.get(stem, "asset_" + str(len(tasks)))
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE_DIR, filename))
    task.set_editor_property("destination_path", DEST_PATH)
    task.set_editor_property("destination_name", dest_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)
    unreal.log("HPWH_IMPORT queued {} -> {}".format(filename, dest_name))

try:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    unreal.log("HPWH_IMPORT import_tasks done")
except Exception as exc:
    unreal.log_error("HPWH_IMPORT import failed: {}".format(exc))

try:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path(DEST_PATH, recursive=True)
    unreal.log("HPWH_IMPORT assets_found={}".format(len(assets)))
    for asset_data in assets:
        unreal.log("HPWH_IMPORT asset={} class={}".format(
            asset_data.asset_name, asset_data.asset_class_path.asset_name))
except Exception as exc:
    unreal.log_error("HPWH_IMPORT report failed: {}".format(exc))
