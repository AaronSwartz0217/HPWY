"""Run with Blender 3.6 --background --factory-startup --python this_file.

Splits selected MIT-licensed Marian42 building modules into independent FBXs.
No upstream scripts/assemblies are executed. Original cell pivot is preserved;
all layout/gallery translations and rotations are baked out of each export.
"""
import bpy
import hashlib
import json
from pathlib import Path
from mathutils import Matrix

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'SourceArt/ProceduralGeneration/WFC/ThirdParty/Marian42'
TARGET = ROOT / 'SourceArt/ProceduralGeneration/WFC/Prepared'
SELECTED = {
    'Floor': 'SM_WFC_Floor',
    'Wall': 'SM_WFC_Wall',
    'Wall Corner 2': 'SM_WFC_WallCorner',
    'Wall Door': 'SM_WFC_Door',
    'Wall Window': 'SM_WFC_Window',
    'Interior Floor': 'SM_WFC_InteriorFloor',
    'Water Fountain': 'SM_WFC_Fountain',
    'Roof Single': 'SM_WFC_Roof',
    'Stairs': 'SM_WFC_Stairs',
    'Enclosed Stairs': 'SM_WFC_EnclosedStairs',
    'Enclosed Stairs Lower': 'SM_WFC_StairsLower',
    'Enclosed Stairs Ceiling': 'SM_WFC_StairsCeiling',
}

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1.0
bpy.ops.import_scene.fbx(filepath=str(SOURCE / 'blocks.fbx'))
originals = {obj.name: obj for obj in bpy.context.scene.objects if obj.type == 'MESH'}
TARGET.mkdir(parents=True, exist_ok=True)
manifest = []
for source_name, output_name in SELECTED.items():
    original = originals[source_name]
    pivot = original.matrix_world.translation.copy()
    mesh = original.data.copy()
    mesh.transform(Matrix.Translation(-pivot) @ original.matrix_world)
    mesh.materials.clear()
    for polygon in mesh.polygons:
        polygon.material_index = 0
    obj = bpy.data.objects.new(output_name, mesh)
    bpy.context.collection.objects.link(obj)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = TARGET / (output_name + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
        object_types={'MESH'}, add_leaf_bones=False, bake_anim=False,
        axis_forward='-Y', axis_up='Z', apply_unit_scale=True,
        use_mesh_modifiers=True, mesh_smooth_type='FACE', path_mode='STRIP')
    manifest.append({'source_object': source_name, 'file': path.name,
        'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
        'removed_world_pivot_m': list(pivot),
        'local_min_m': [min(v.co[i] for v in mesh.vertices) for i in range(3)],
        'local_max_m': [max(v.co[i] for v in mesh.vertices) for i in range(3)]})
    bpy.data.objects.remove(obj, do_unlink=True)
(TARGET / 'conversion_manifest.json').write_text(
    json.dumps({'upstream_commit': '8750c537e6c8f95806ba709e2ae5d940fd51bff3',
        'tool': bpy.app.version_string, 'units': 'meters; FBX unit metadata converted by UE',
        'note': 'Cell pivot preserved; flat source floor is -0.6 m relative to pivot. '
                'Example TileSet scales meshes by 1.5 and translates +90 cm to place floors at Z=0.',
        'models': manifest}, indent=2), encoding='utf-8')
print('SPIRIT_WFC_EXPORTED', len(manifest), 'modules to', TARGET)
