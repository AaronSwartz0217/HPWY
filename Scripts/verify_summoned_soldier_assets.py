"""Read native defaults and animation dependencies only; no game/PIE/world spawning."""
import json
from pathlib import Path
import unreal

checks = []
def check(name, condition):
    checks.append({"check": name, "passed": bool(condition)})

cls = unreal.load_class(None, "/Script/CrashCourse.CC_SummonedSoldier")
assert cls
defaults = unreal.get_default_object(cls)
form = defaults.get_editor_property("soldier_form")
stages = form.get_editor_property("stages") if form else []
check("bronze_form_has_initial_stage", bool(stages))
mesh_component = defaults.get_editor_property("mesh")
capsule = defaults.get_editor_property("capsule_component")
offset = mesh_component.get_editor_property("relative_location")
half_height = capsule.get_editor_property("capsule_half_height")
check("mesh_origin_at_capsule_feet", abs(offset.z + half_height) < 0.1)
rotation = mesh_component.get_editor_property("relative_rotation")
check("mesh_forward_matches_player", abs(rotation.yaw + 90) < 0.1)
locomotion = defaults.get_editor_property("locomotion")
library = locomotion.get_editor_property("library")
entries = library.get_editor_property("entries") if library else []
mesh = stages[0].get_editor_property("mesh") if stages else None
entry = next((row for row in entries if row.get_editor_property("mesh") == mesh), None)
check("bronze_mesh_has_locomotion_entry", entry is not None)
if entry:
    check("idle_run_and_attack_configured", all(entry.get_editor_property(field)
          for field in ("idle", "run", "jab", "anim_class")))
    check("stage_uses_same_state_machine", stages[0].get_editor_property("anim_class") == entry.get_editor_property("anim_class"))
    skeleton = mesh.get_editor_property("skeleton")
    check("animations_match_bronze_skeleton", all(entry.get_editor_property(field).get_editor_property("skeleton") == skeleton
          for field in ("idle", "run", "jab")))
check("melee_presentation_configured", defaults.get_editor_property("melee_presentation") is not None)
check("ai_controller_configured", defaults.get_editor_property("ai_controller_class") is not None)
check("summon_has_no_experience_reward", defaults.get_editor_property("experience_reward") == 0)
project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
report = project / "Saved/UI/SummonedSoldierAssetVerification.json"
report.parent.mkdir(parents=True, exist_ok=True)
passed = all(row["passed"] for row in checks)
report.write_text(json.dumps({"passed": passed, "checks": checks,
                             "gameStarted": False, "assetsSaved": False}, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("SUMMONED_SOLDIER_ASSET_CHECK " + str(passed) + " " + str(report))
unreal.SystemLibrary.quit_editor()
