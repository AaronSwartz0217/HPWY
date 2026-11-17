"""Apply nine stage appearances and green/yellow pickups to the existing arena."""
import unreal
import runpy
import pathlib
import json
scripts = pathlib.Path(__file__).parent
runpy.run_path(str(scripts / "configure_three_powers.py"))
runpy.run_path(str(scripts / "configure_npc_art.py"))
ROOT = "/Game/HPWH"
tools = unreal.AssetToolsHelpers.get_asset_tools()
materials = {}
for key, color in [("Bronze_Green", unreal.LinearColor(0.025, 0.8, 0.06, 1)), ("Ceramic_Yellow", unreal.LinearColor(0.95, 0.7, 0.02, 1))]:
    path = ROOT + "/Materials/MI_Pickup_" + key
    material = unreal.load_asset(path) or tools.create_asset("MI_Pickup_" + key, ROOT + "/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(material, unreal.load_asset(ROOT + "/Materials/M_TestTint"))
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material, "Tint", color)
    unreal.EditorAssetLibrary.save_loaded_asset(material, False)
    materials[key] = material
for index, key in [(1, "Bronze_Green"), (2, "Ceramic_Yellow")]:
    bp = unreal.load_asset(ROOT + "/Blueprints/BP_Artifact_%02d" % index)
    unreal.get_default_object(bp.generated_class()).get_editor_property("mesh").set_material(0, materials[key])
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
report = []
for actor in actors.get_all_level_actors():
    if isinstance(actor, unreal.CC_FormArtifact):
        form = actor.get_editor_property("artifact").get_editor_property("form")
        identity = str(form.get_editor_property("form_id")) if form else ""
        key = {"Artifact_01": "Bronze_Green", "Artifact_02": "Ceramic_Yellow"}.get(identity)
        if key:
            actor.get_editor_property("mesh").set_material(0, materials[key])
            actor.set_actor_label("Pickup_" + key)
            report.append({"form": identity, "material": materials[key].get_path_name()})
    if isinstance(actor, unreal.TextRenderActor):
        label = actor.get_actor_label()
        if label.startswith("Style 1") or "Bronze" in label:
            actor.get_editor_property("text_render").set_text("BRONZE POWER - GREEN [E]")
        elif label.startswith("Style 2") or "Ceramic" in label:
            actor.get_editor_property("text_render").set_text("CERAMIC POWER - YELLOW [E]")
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
pathlib.Path(unreal.Paths.project_saved_dir() + "FormRoutePickups.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("FORM_ROUTES_READY")
