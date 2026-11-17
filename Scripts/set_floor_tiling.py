import unreal

DEST_PATH = "/Game/HPWH/Assets/Materials"
TEXTURE_NAME = "T_HPWH_Floor"
MATERIAL_NAME = "M_HPWH_Floor"
MATERIAL_PATH = DEST_PATH + "/" + MATERIAL_NAME

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
texture = unreal.EditorAssetLibrary.load_asset(DEST_PATH + "/" + TEXTURE_NAME)
if texture is None:
    print("HPWH_FLOOR texture missing")
else:
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        unreal.EditorAssetLibrary.delete_asset(MATERIAL_PATH)

    material = asset_tools.create_asset(MATERIAL_NAME, DEST_PATH, unreal.Material,
        unreal.MaterialFactoryNew())

    uv = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureCoordinate, -1000, 0)
    uv.set_editor_property("u_tiling", 0.6667)
    uv.set_editor_property("v_tiling", 0.6667)

    # 固定旋转 90°，避免 Rotator 随时间持续旋转。
    custom = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionCustom, -700, 0)
    custom.set_editor_property("code", "float2 c = uv - 0.5; float s = 1.0; float co = 0.0; return float2(c.x * co - c.y * s, c.x * s + c.y * co) + 0.5;")
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    uv_input = unreal.CustomInput()
    uv_input.set_editor_property("input_name", "uv")
    custom.set_editor_property("inputs", [uv_input])
    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", custom, "uv")

    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -420, 0)
    sample.set_editor_property("texture", texture)
    unreal.MaterialEditingLibrary.connect_material_expressions(custom, "", sample, "UVs")
    unreal.MaterialEditingLibrary.connect_material_property(
        sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -420, 220)
    roughness.set_editor_property("r", 0.85)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(material.get_path_name())
    print("HPWH_FLOOR recreated with tiling 8")
