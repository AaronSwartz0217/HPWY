import unreal
import json

report = {"widget_blueprints": [], "characters": [], "blueprint_graph_access": []}
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
for asset in registry.get_assets_by_path("/Game", recursive=True):
    if "WidgetBlueprint" in str(asset.asset_class_path):
        report["widget_blueprints"].append(str(asset.package_name))
for path in ["/Game/THAT/BP_PlayerCharacter", "/Game/THAT/BP_EnemyCharacter", "/Game/HPWH/Blueprints/BP_ProgressionCharacter", "/Game/HPWH/Blueprints/BP_ProgressionEnemy"]:
    bp = unreal.load_asset(path)
    cdo = unreal.get_default_object(bp.generated_class())
    mesh = cdo.get_editor_property("mesh")
    entry = {"path": path, "mesh": str(mesh.get_editor_property("skeletal_mesh_asset")), "anim": str(mesh.get_editor_property("anim_class")), "widgets": []}
    for component in cdo.get_components_by_class(unreal.WidgetComponent):
        data = {"name": component.get_name(), "class": component.get_class().get_name(), "widget_class": str(component.get_editor_property("widget_class"))}
        if isinstance(component, unreal.CC_WidgetComponent):
            data["attribute_map"] = str(component.get_editor_property("attribute_map"))
        entry["widgets"].append(data)
    report["characters"].append(entry)
    try:
        report["blueprint_graph_access"].append({"path": path, "graphs": str(bp.get_editor_property("ubergraph_pages"))})
    except Exception as error:
        report["blueprint_graph_access"].append({"path": path, "unavailable": str(error)})
report_path = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir() + "UIInspection.json")
with open(report_path, "w", encoding="utf-8") as stream:
    json.dump(report, stream, ensure_ascii=False, indent=2)
unreal.log("UI_INSPECTION_OK: " + report_path)
