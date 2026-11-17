"""Edit only progression data/defaults; never save maps or imported art packages."""
import unreal
import json

ROOT = "/Game/HPWH"
edited = []
forms = []
names = ["自然之力", "青铜之力", "陶塑之力", "神秘之力", "墨化之力"]
routes = [["竹路人", "竹节人", "竹拳击手"], ["铜机械小兵", "铜械战士", "铜械将军"], ["陶童子", "陶土行者", "陶土僧人"]]
bronze = unreal.load_asset("/Game/SOURCE/小怪/铜机械小兵/SK_铜机械小兵")
assert isinstance(bronze, unreal.SkeletalMesh), "Bronze skeletal mesh is missing"
for index in range(5):
    key = "Native" if index == 0 else "Artifact_%02d" % index
    form = unreal.load_asset(ROOT + "/Forms/DA_Form_" + key)
    assert form
    form.set_editor_property("display_name", names[index])
    form.set_editor_property("available", index < 3)
    stages = form.get_editor_property("stages")
    for tier, stage in enumerate(stages):
        stage.set_editor_property("cost", [0, 100, 1000][tier])
        if index < 3:
            model = routes[index][tier]
            mesh = unreal.load_asset("/Game/SOURCE/小怪/" + model + "/SK_" + model)
            assert mesh
            scale = 180.0 / (mesh.get_bounds().box_extent.z * 2.0)
            stage.set_editor_property("mesh", mesh)
            stage.set_editor_property("mesh_scale", unreal.Vector(scale, scale, scale))
            stage.set_editor_property("anim_class", None)
        stages[tier] = stage
    form.set_editor_property("stages", stages)
    forms.append(form)
    edited.append(form)

for name in ["BP_ProgressionEnemy", "BP_TestDummy", "BP_PatrolEnemy"]:
    bp = unreal.load_asset(ROOT + "/Blueprints/" + name)
    obj = unreal.get_default_object(bp.generated_class())
    obj.set_editor_property("experience_reward", 10)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    edited.append(bp)

# Keep stable IDs and existing scene references; disabled forms are rejected by C++.
for asset in edited:
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
report = {"forms": [{"id": str(f.get_editor_property("form_id")), "name": str(f.get_editor_property("display_name")), "available": f.get_editor_property("available"), "actualCosts": [s.get_editor_property("cost") for s in f.get_editor_property("stages")], "meshes": [s.get_editor_property("mesh").get_path_name() if s.get_editor_property("mesh") else None for s in f.get_editor_property("stages")]} for f in forms],
          "costs": [0, 100, 1000], "enemyXP": 10, "bronzeMesh": bronze.get_path_name(),
          "meshInspection": [n for n in dir(bronze) if "bound" in n or "skeleton" in n]}
with open(unreal.Paths.project_saved_dir() + "ThreePowersSetup.json", "w", encoding="utf-8") as output:
    json.dump(report, output, ensure_ascii=False, indent=2)
unreal.log("THREE_POWERS_SETUP_OK: " + str(report))
