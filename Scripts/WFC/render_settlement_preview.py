"""Offline Blender preview of the actual UE-generated cell JSON, not a game run."""
import bpy
import json
import math
import sys
from pathlib import Path
from mathutils import Matrix,Vector

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'SourceArt/ProceduralGeneration/WFC/SettlementKit'
REPORT=json.loads((ROOT/('Saved/WFC/GridEditorCreation.json' if '--painted' in sys.argv else 'Saved/WFC/SettlementCreation.json')).read_text())
MANIFEST=json.loads((SOURCE/'kit_manifest.json').read_text())
OUT=ROOT/'Plugins/SpiritWFC/Docs/Previews'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
meshes={}
for row in MANIFEST['models']:
    if row['mesh'].endswith('AxisProbe'):continue
    before=set(bpy.context.scene.objects)
    bpy.ops.import_scene.fbx(filepath=str(SOURCE/(row['mesh']+'.fbx')))
    obj=next(o for o in bpy.context.scene.objects if o not in before and o.type=='MESH')
    mesh=obj.data.copy();mesh.transform(obj.matrix_world)
    if MANIFEST.get('fbx_y_mirrored'):
        mesh.transform(Matrix.Diagonal((1,-1,1,1)))
        # Mirrored coordinate correction reverses handedness.
        import bmesh
        bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free()
    meshes[row['mesh']]=mesh
    for item in list(bpy.context.scene.objects):
        if item not in before:bpy.data.objects.remove(item,do_unlink=True)

for mat in bpy.data.materials:
    mat.use_nodes=True
    shader=mat.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value=mat.diffuse_color
    shader.inputs['Roughness'].default_value=.86

scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE';scene.eevee.use_gtao=True
scene.eevee.gtao_distance=2;scene.eevee.gtao_factor=1.25;scene.eevee.taa_render_samples=64
scene.render.resolution_x=1440;scene.render.resolution_y=1050;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Preview Backdrop');scene.world.color=(.65,.67,.69)
scene.view_settings.view_transform='Standard';scene.view_settings.look='Medium High Contrast'
scene.view_settings.exposure=0;scene.view_settings.gamma=1
sun_data=bpy.data.lights.new('Soft Sun','SUN');sun_data.energy=2.4;sun_data.angle=.15
sun=bpy.data.objects.new('Soft Sun',sun_data);scene.collection.objects.link(sun);sun.rotation_euler=(.48,-.42,-.55)
fill_data=bpy.data.lights.new('Fill','AREA');fill_data.energy=24000;fill_data.size=70
fill=bpy.data.objects.new('Fill',fill_data);scene.collection.objects.link(fill);fill.location=(42,42,65)
camera_data=bpy.data.cameras.new('Camera');camera=bpy.data.objects.new('Camera',camera_data)
scene.collection.objects.link(camera);scene.camera=camera;camera_data.type='ORTHO';camera_data.ortho_scale=135
target=Vector((39,39,1));camera.location=target+Vector((68,-85,95))
camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler()

for entry in REPORT['maps']:
    items=[]
    for cell in entry['layout']:
        if not cell['render']:continue
        obj=bpy.data.objects.new(cell['tile'],meshes[cell['mesh']]);scene.collection.objects.link(obj)
        x,y,z=cell['xyz'];obj.location=(x*6,y*6,z*3.6);obj.rotation_euler.z=cell['yaw']*math.pi/2
        items.append(obj)
    suffix='ThreeLayers' if entry['map'].endswith('ThreeLayers') else 'Planar'
    scene.render.filepath=str(OUT/((entry['preview_name'] if entry.get('preview_name') else 'Settlement_'+suffix)+'.png'))
    bpy.ops.render.render(write_still=True)
    for obj in items:bpy.data.objects.remove(obj,do_unlink=True)
print('SETTLEMENT_PREVIEWS',OUT)
