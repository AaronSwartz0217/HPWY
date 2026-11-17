"""Run in Blender background mode; inspect the downloaded FBX, never execute upstream scripts."""
import bpy
import json
from pathlib import Path
from mathutils import Vector

project = Path(__file__).resolve().parents[2]
folder = project / "SourceArt/ProceduralGeneration/WFC/ThirdParty/Marian42"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(folder / "blocks.fbx"))
rows = []
for obj in bpy.context.scene.objects:
    if obj.type != "MESH":
        continue
    points = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    rows.append({"name": obj.name, "vertices": len(obj.data.vertices), "polygons": len(obj.data.polygons),
                 "dimensions": list(obj.dimensions), "location": list(obj.location),
                 "minimum": [min(p[i] for p in points) for i in range(3)],
                 "maximum": [max(p[i] for p in points) for i in range(3)]})
out = project / "Saved/WFC/ReferenceFBXInventory.json"
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(rows, indent=2), encoding="utf-8")
print("WFC_REFERENCE_MESH_COUNT", len(rows))
