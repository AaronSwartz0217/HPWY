import unreal

TEXTURE_SOURCE = r"C:\Users\29717\Desktop\幻魄物语\场景物体设定图\地板pbr.png"
DEST_PATH = "/Game/HPWH/Assets/Materials"
TEXTURE_NAME = "T_HPWH_Floor"
MATERIAL_NAME = "M_HPWH_Floor"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

task = unreal.AssetImportTask()
task.set_editor_property("filename", TEXTURE_SOURCE)
task.set_editor_property("destination_path", DEST_PATH)
task.set_editor_property("destination_name", TEXTURE_NAME)
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
asset_tools.import_asset_tasks([task])

texture = unreal.EditorAssetLibrary.load_asset(DEST_PATH + "/" + TEXTURE_NAME)
if texture is None:
    print("HPWH_FLOOR texture import failed")
else:
    material = unreal.EditorAssetLibrary.load_asset(DEST_PATH + "/" + MATERIAL_NAME)
    if material is None:
        material = asset_tools.create_asset(MATERIAL_NAME, DEST_PATH, unreal.Material,
            unreal.MaterialFactoryNew())

    uv = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureCoordinate, -760, 0)
    uv.set_editor_property("u_tiling", 6.0)
    uv.set_editor_property("v_tiling", 6.0)

    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -420, 0)
    sample.set_editor_property("texture", texture)
    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", sample, "UVs")
    unreal.MaterialEditingLibrary.connect_material_property(
        sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -420, 220)
    roughness.set_editor_property("r", 0.85)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(material.get_path_name())
    print("HPWH_FLOOR material created: " + material.get_path_name())
