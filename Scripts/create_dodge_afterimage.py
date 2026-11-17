"""Create the translucent pose-snapshot material used by the dodge."""
import unreal

folder = "/Game/HPWH/Effects/Dodge"
name = "M_DodgeAfterimage"
material = unreal.EditorAssetLibrary.load_asset(folder + "/" + name)
if not material:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, folder, unreal.Material, unreal.MaterialFactoryNew())
assert material
edit = unreal.MaterialEditingLibrary
edit.delete_all_material_expressions(material)
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
edit.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
color = edit.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -350, -100)
color.set_editor_property("constant", unreal.LinearColor(.85, .66, .32, 1.0))
edge = edit.create_material_expression(material, unreal.MaterialExpressionFresnel, -640, 80)
edge.set_editor_property("base_reflect_fraction", .38)
opacity = edit.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -640, 220)
opacity.set_editor_property("parameter_name", "GhostOpacity")
opacity.set_editor_property("default_value", .37)
multiply = edit.create_material_expression(material, unreal.MaterialExpressionMultiply, -350, 120)
edit.connect_material_expressions(edge, "", multiply, "A")
edit.connect_material_expressions(opacity, "", multiply, "B")
edit.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
edit.connect_material_property(multiply, "", unreal.MaterialProperty.MP_OPACITY)
edit.recompile_material(material)
assert unreal.EditorAssetLibrary.save_loaded_asset(material, False)
unreal.log("DODGE_AFTERIMAGE_MATERIAL_READY")
