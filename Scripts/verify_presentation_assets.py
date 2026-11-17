"""Read-only editor verification. Does not enter PIE, start a game, or save art assets."""
import json
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
settings = unreal.get_default_object(unreal.load_class(None, "/Script/UnrealEd.LevelEditorMiscSettings"))
auto_nav = settings.get_editor_property("bNavigationAutoUpdate")
settings.set_editor_property("bNavigationAutoUpdate", False)
issues = []
maps = []
checked = set()
try:
    for path in ("/Game/LanTuRebuild/Maps/初始房间/restartlife", "/Game/LanTuRebuild/Maps/古风小镇/古风小镇", "/Game/HPWH/Maps/L_FlatTestArena"):
        assert levels.load_level(path)
        maps.append(path)
        for actor in actors.get_all_level_actors():
            for component in actor.get_components_by_class(unreal.StaticMeshComponent):
                mesh = component.get_editor_property("static_mesh")
                if not mesh or not mesh.get_editor_property("nanite_settings").get_editor_property("enabled"):
                    continue
                materials = [slot.get_editor_property("material_interface") for slot in mesh.get_editor_property("static_materials")]
                materials += [component.get_material(i) for i in range(component.get_num_materials())]
                for material in materials:
                    if not material:
                        continue
                    key = (mesh.get_path_name(), material.get_path_name())
                    if key in checked:
                        continue
                    checked.add(key)
                    if not unreal.CC_AnimationSetupLibrary.is_nanite_blend_supported(material):
                        issues.append({"mesh": key[0], "material": key[1], "issue": "Unsupported Nanite blend"})
                    base = material
                    seen = set()
                    while isinstance(base, unreal.MaterialInstanceConstant) and base.get_path_name() not in seen:
                        seen.add(base.get_path_name())
                        base = base.get_editor_property("parent")
                    # Built-in /Engine special materials are handled internally by the renderer.
                    if isinstance(base, unreal.Material) and not base.get_path_name().startswith("/Engine/") and not base.get_editor_property("used_with_nanite"):
                        issues.append({"mesh": key[0], "material": key[1], "issue": "Missing Nanite usage", "base": base.get_path_name()})
finally:
    settings.set_editor_property("bNavigationAutoUpdate", auto_nav)

gi = unreal.get_default_object(unreal.load_class(None, "/Script/CrashCourse.CC_AdventureGameInstance"))
music = gi.get_editor_property("background_music")
home = unreal.load_asset("/Game/UI/MainMenu/DA_HomeArt")
logo = home.get_editor_property("startup_logo")
checks = {
    "threeMapsLoaded": len(maps) == 3,
    "naniteMaterialReferencesValid": not issues,
    "correctMusic": music and music.get_name() == "SW_ExhibitionTheme",
    "musicLoops": music and music.get_editor_property("looping"),
    "startupLogoPresent": isinstance(logo, unreal.Texture2D),
    "runtimeDebugOverlayHiddenByDefault": not gi.get_editor_property("show_runtime_debug_messages"),
}
report = {"passed": all(checks.values()), "checks": checks, "maps": maps,
          "naniteMeshMaterialPairsChecked": len(checked), "materialIssues": issues,
          "gameStarted": False, "artAssetsSaved": False,
          "runtimeVerification": "Not run: automatic approval limits execution to compilation and asset inspection."}
out = project / "Saved/UI/PresentationAssetVerification.json"
out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("PRESENTATION_ASSET_VERIFICATION_READY " + str(out))
assert report["passed"], checks
unreal.SystemLibrary.quit_editor()
