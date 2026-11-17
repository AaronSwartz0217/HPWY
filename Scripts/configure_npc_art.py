"""Assign retargeted bronze/bamboo art to arena NPCs and the bronze player form."""
import unreal
import json
import pathlib
import shutil
ROOT = "/Game/HPWH"
names = ["铜机械小兵", "铜械战士", "铜械将军", "铜械降神", "竹节人", "竹路人", "竹拳击手", "陶罐头", "陶童子", "陶土行者", "陶土僧人"]
path = ROOT + "/Locomotion/DA_NPCLocomotion"
library = unreal.load_asset(path)
if not library:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.CC_LocomotionLibrary)
    library = unreal.AssetToolsHelpers.get_asset_tools().create_asset("DA_NPCLocomotion", ROOT + "/Locomotion", unreal.CC_LocomotionLibrary, factory)
entries = []
meshes = []
validation = []
for name in names:
    mesh = unreal.load_asset("/Game/SOURCE/小怪/" + name + "/SK_" + name)
    entry = unreal.CC_MeshLocomotion()
    entry.set_editor_property("mesh", mesh)
    poses = []
    rotations = []
    for field in ["idle", "run"]:
        clip = unreal.load_asset(ROOT + "/Locomotion/Animations/A_" + name + "_" + field.capitalize())
        assert clip and clip.get_editor_property("skeleton") == mesh.get_editor_property("skeleton")
        entry.set_editor_property(field, clip)
        if field == "run":
            for time in [0.0, 0.2]:
                pose = unreal.AnimationLibrary.get_bone_pose_for_time(clip, "LeftLeg", time, False)
                poses.append(str(pose))
                rotations.append(pose.rotation)
    assert sum(abs(getattr(rotations[0], axis) - getattr(rotations[1], axis)) for axis in ["x", "y", "z", "w"]) > 0.01, "Run leg pose does not change: " + name
    entries.append(entry)
    meshes.append(mesh)
    validation.append({"name": name, "legPoses": poses})
library.set_editor_property("entries", entries)
unreal.EditorAssetLibrary.save_loaded_asset(library, False)
for name in ["BP_ProgressionCharacter", "BP_PatrolEnemy", "BP_TestDummy", "BP_ProgressionEnemy"]:
    bp = unreal.load_asset(ROOT + "/Blueprints/" + name)
    obj = unreal.get_default_object(bp.generated_class())
    obj.get_editor_property("locomotion").set_editor_property("library", library)
    if name != "BP_ProgressionCharacter": obj.set_editor_property("experience_reward", 10)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
# Three-tier appearances are owned by configure_three_powers.py.
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level(ROOT + "/Maps/L_FlatTestArena")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
report = []
index = 0
for actor in sorted(actors.get_all_level_actors(), key=lambda a: a.get_actor_label()):
    if isinstance(actor, unreal.CC_EnemyCharacter):
        order = [0, 4, 1, 7, 2, 3, 5, 6, 7, 0]
        mesh = meshes[order[index % len(order)]]
        size = 180.0 / (mesh.get_bounds().box_extent.z * 2.0)
        component = actor.get_editor_property("mesh")
        component.set_editor_property("skeletal_mesh_asset", mesh)
        component.set_editor_property("anim_class", None)
        component.set_relative_scale3d(unreal.Vector(size, size, size))
        actor.get_editor_property("locomotion").set_editor_property("library", library)
        actor.set_editor_property("experience_reward", 10)
        report.append({"actor": actor.get_actor_label(), "mesh": mesh.get_path_name()})
        index += 1
    if isinstance(actor, unreal.CC_FormArtifact):
        form = actor.get_editor_property("artifact").get_editor_property("form")
        enabled = form.get_editor_property("available")
        actor.set_actor_hidden_in_game(not enabled)
        actor.set_actor_enable_collision(enabled)
        actor.get_editor_property("artifact").set_editor_property("enabled", enabled)
    if isinstance(actor, unreal.TextRenderActor):
        label = actor.get_actor_label()
        if label.startswith("TARGET DUMMIES"):
            actor.get_editor_property("text_render").set_text("TARGET DUMMIES | 10 XP")
        elif label.startswith(("Style 3", "Style 4")):
            actor.set_actor_hidden_in_game(True)
        elif label.startswith("Style 1"):
            actor.get_editor_property("text_render").set_text("Bronze Power [E]")
        elif label.startswith("Style 2"):
            actor.get_editor_property("text_render").set_text("Ceramic Power [E]")
project = pathlib.Path(unreal.Paths.project_dir())
map_file = project / "Content/HPWH/Maps/L_FlatTestArena.umap"
backup = project / "Saved/SourceNamingBackup/Content/HPWH/Maps/L_FlatTestArena.umap"
backup.parent.mkdir(parents=True, exist_ok=True)
if not backup.exists(): shutil.copy2(map_file, backup)
levels.save_current_level()
(project / "Saved/NPCArtSetup.json").write_text(json.dumps({"npcs": report, "animations": validation}, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("NPC_ART_SETUP_OK: " + str(report))
