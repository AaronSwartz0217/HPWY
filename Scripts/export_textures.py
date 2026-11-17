import unreal

items = [
    ("/Game/HPWH/Assets/Models/书堆/Textures/书堆_texture_0", r"C:\HPWH\Saved\book_basecolor.png"),
    ("/Game/HPWH/Assets/Models/八卦镜/Textures/八卦镜_texture_0", r"C:\HPWH\Saved\mirror_basecolor.png"),
]

tasks = []
for asset_path, filename in items:
    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if texture is None:
        print("HPWH_TEX missing " + asset_path)
        continue
    task = unreal.AssetExportTask()
    task.set_editor_property("object", texture)
    task.set_editor_property("filename", filename)
    task.set_editor_property("automated", True)
    task.set_editor_property("exporter", unreal.TextureExporter())
    tasks.append(task)

if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().export_asset_tasks(tasks)
print("HPWH_TEX export done")
