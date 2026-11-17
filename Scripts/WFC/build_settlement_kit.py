"""Author a small, coherent architectural kit in Blender; no third-party code.

Units meters, cell 6m, storey 3.6m, mesh pivots at walkable floor. Roads contain
actual paving/curbs. Houses contain walls, door/window openings, timber and roof.
Multi-storey pieces use matching floor openings and a U stair with 18cm risers.
"""
import bpy
import sys
import json
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(sys.argv[sys.argv.index('--output-dir')+1]) if '--output-dir' in sys.argv else ROOT/'SourceArt/ProceduralGeneration/WFC/SettlementKit'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1
COLORS = [(0.20,0.25,0.18,1),(0.56,0.53,0.43,1),(0.76,0.69,0.53,1),
          (0.19,0.12,0.07,1),(0.16,0.23,0.22,1),(0.42,0.42,0.35,1)]
MATS=[]
for i,color in enumerate(COLORS):
    m=bpy.data.materials.new('WFC_Palette_'+str(i));m.diffuse_color=color;MATS.append(m)
VERTS=[];FACES=[];SLOTS=[];MANIFEST=[]


def box(x0,x1,y0,y1,z0,z1,mat=1):
    n=len(VERTS)
    VERTS.extend([(x0,y0,z0),(x1,y0,z0),(x1,y1,z0),(x0,y1,z0),
                  (x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1)])
    for f in ((0,3,2,1),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7),(4,5,6,7)):
        FACES.append(tuple(n+i for i in f));SLOTS.append(mat)


def roof(front=-2.75):
    # Closed pitched roof with deep eaves, ridge and timber gables.
    n=len(VERTS)
    VERTS.extend([(-2.75,front,3.05),(2.75,front,3.05),(0,front,4.25),
                  (-2.75,2.75,3.05),(2.75,2.75,3.05),(0,2.75,4.25)])
    for face,slot in [((0,3,5,2),4),((1,2,5,4),4),((0,2,1),2),((3,4,5),2),((0,1,4,3),3)]:
        FACES.append(tuple(n+i for i in reversed(face)));SLOTS.append(slot)
    box(-0.08,0.08,front-.09,2.84,4.23,4.34,3)
    for y in (front,2.65): box(-2.8,2.8,y,y+0.1,2.97,3.10,3)


def wall(axis,side,door=False,link=False,tangent_min=-2.25):
    # Opening uses local wall tangent; the front door sits on the right corridor.
    lo,hi=(-0.75,0.75) if link else (0.65,1.95) if door else (-0.75,0.75)
    low,high=(0,2.45) if door or link else (1.1,2.15)
    zmax=3.0 if ROOF else 3.6
    def segment(a,b,z0,z1,mat=2):
        if axis=='x': box(side-.08,side+.08,a,b,z0,z1,mat)
        else: box(a,b,side-.08,side+.08,z0,z1,mat)
    segment(tangent_min,lo,0,zmax);segment(hi,2.25,0,zmax)
    if low: segment(lo,hi,0,low)
    segment(lo,hi,high,zmax)
    segment(lo-.065,lo+.035,low,high,3);segment(hi-.035,hi+.065,low,high,3)
    segment(lo-.08,hi+.08,high,high+.09,3)
    if not (door or link):
        # Sparse lattice in an open window; no opaque glass collision sheet.
        for a in (lo+.3,(lo+hi)/2,hi-.3): segment(a-.025,a+.025,low,high,3)
        segment(lo,hi,low-.05,low+.03,3)
    if door or link:
        segment(tangent_min-.08,lo,.12,.24,3);segment(hi,2.33,.12,.24,3)
    else:segment(tangent_min-.08,2.33,.12,.24,3)


def floor(opening=False,front=-2.25):
    if not opening: box(-2.25,2.25,front,2.25,-.12,0,1)
    else:
        # Stair void reaches the front; lower storey's top landing bridges to
        # the right corridor. It does not leave a low ceiling over the exit.
        box(.8,2.25,-2.25,2.25,-.12,0,1)
        box(-2.25,-2.15,-2.25,2.25,-.12,0,1)
        box(-2.15,.8,2.15,2.25,-.12,0,1)


def stairs():
    # A low entrance landing clears the front wall while turning onto the first flight.
    box(-.8,1.95,-2.15,-1.30,0,.18,1)
    for i in range(10):
        box(-2.1,-.8,-1.9+i*.3,-1.6+i*.3,(i+1)*.18-.12,(i+1)*.18,1)
        # Keep the return flight hollow: a solid-to-ground box blocks access to the first flight.
        top=1.8+(i+1)*.18
        box(-.6,.7,.8-i*.3,1.1-i*.3,top-.12,top,1)
    box(-2.1,.7,1.1,2.1,1.68,1.8,1)
    # The upper module floor starts at x=.8. Join edges without coplanar faces.
    box(-.6,.8,-2.1,-1.3,3.48,3.6,1)
    # Stair edge posts; avoid blocking the turning/exit path with cross rails.
    for x in (-2.12,.73):
        for y,z in ((1.9,1.8),(-1.8 if x<0 else -2.10,0 if x<0 else 3.6)):
            box(x-.03,x+.03,y-.03,y+.03,z,z+.85,3)


def building(kind,variant):
    opening=kind in ('Middle','Top')
    floor(opening,front=-.8 if kind=='CompoundWing' else -2.25)
    for axis,side in (('x',-2.25),('x',2.25),('y',-.8 if kind=='CompoundWing' else -2.25),('y',2.25)):
        front=axis=='y' and side<0 and kind in ('House','Base','CompoundHouse','CompoundBase')
        link=axis=='x' and ((side>0 and kind in ('CompoundHouse','CompoundBase')) or (side<0 and kind=='CompoundWing'))
        wall(axis,side,door=front,link=link,tangent_min=-.8 if kind=='CompoundWing' and axis=='x' else -2.25)
    height=3.0 if ROOF else 3.6
    for x in (-2.25,2.25):
        for y in ((-.8 if kind=='CompoundWing' else -2.25),2.25): box(x-.10,x+.10,y-.10,y+.10,0,height,3)
    if kind in ('House','Base','CompoundHouse','CompoundBase','CompoundWing'):
        box(-3,3,-3,3,-.2,-.12,5)
        box(.55,2.05,-3,-2.25,-.02,.025,1)
    if kind in ('Base','Middle','CompoundBase'): stairs()
    if kind in ('CompoundHouse','CompoundBase','CompoundWing'):
        lo,hi=(2.25,3.0) if kind!='CompoundWing' else (-3.0,-2.25)
        box(lo,hi,-.75,.75,-.12,0,1)
        box(lo,hi,-.84,-.75,0,2.55,2);box(lo,hi,.75,.84,0,2.55,2)
        box(lo,hi,-.9,.9,2.5,2.65,4)
    if ROOF: roof(front=-1.3 if kind=='CompoundWing' else -2.75)
    if variant:
        # Secondary facade rhythm, compatible with the same footprint/door.
        front_y=-.8 if kind=='CompoundWing' else -2.25
        for x in (-1.65,-.85): box(x-.045,x+.045,front_y-.11,front_y,.25,2.9,3)


def ground(mask=None,plaza=False,grout_geometry=True):
    box(-3,3,-3,3,-.2,0,1 if plaza else 0)
    if mask is not None:
        box(-1.05,1.05,-1.05,1.05,.005,.04,1)
        for d in range(4):
            if not mask&(1<<d):continue
            if d<2:
                a,b=(1.05,3) if d==0 else (-3,-1.05)
                box(a,b,-1.05,1.05,.005,.04,1)
                for y in (-1.16,1.06):box(a,b,y,y+.1,.01,.14,5)
            else:
                a,b=(1.05,3) if d==2 else (-3,-1.05)
                box(-1.05,1.05,a,b,.005,.04,1)
                for x in (-1.16,1.06):box(x,x+.1,a,b,.01,.14,5)
        # Very shallow joints make paving readable without texture noise.
        for a in (-.7,0,.7):box(-1.04,1.04,a-.012,a+.012,.04,.045,5)
    if plaza and grout_geometry:
        for a in (-2,-1,0,1,2):
            box(-2.98,2.98,a-.012,a+.012,.001,.007,5)
            box(a-.012,a+.012,-2.98,2.98,.001,.007,5)


def export(name):
    # UE's FBX scene conversion flips Y. Bake the inverse and restore winding
    # so authored +X/-Y sockets agree with actual UE geometry and collision.
    mesh=bpy.data.meshes.new(name)
    mesh.from_pydata([(x,-y,z) for x,y,z in VERTS],[],[tuple(reversed(f)) for f in FACES]);mesh.update()
    obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj)
    for mat in MATS:mesh.materials.append(mat)
    for poly,slot in zip(mesh.polygons,SLOTS):poly.material_index=slot
    bevel=obj.modifiers.new('Small edge bevel','BEVEL');bevel.width=.02;bevel.segments=1
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    mesh=obj.data;uv=mesh.uv_layers.new(name='UVMap')
    for poly in mesh.polygons:
        drop=max(range(3),key=lambda axis:abs(poly.normal[axis]))
        axes=[axis for axis in range(3) if axis!=drop]
        for loop in poly.loop_indices:
            point=mesh.vertices[mesh.loops[loop].vertex_index].co
            uv.data[loop].uv=(point[axes[0]]*.25,point[axes[1]]*.25)
    bpy.ops.export_scene.fbx(filepath=(OUT/(name+'.fbx')).as_posix(),use_selection=True,object_types={'MESH'},
        add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,
        mesh_smooth_type='FACE',path_mode='STRIP')
    MANIFEST.append({'mesh':name,'vertices':len(mesh.vertices),'faces':len(mesh.polygons)})
    bpy.data.objects.remove(obj,do_unlink=True);VERTS.clear();FACES.clear();SLOTS.clear()



if __name__ == "__main__":
    ROOF=False
    ground();export('SM_Settlement_Ground')
    for mask in range(16):
        ground(mask);export('SM_Settlement_Road_%02d'%mask)
    ground(plaza=True);export('SM_Settlement_Plaza')
    for kind in ('House','Base','Middle','Top','CompoundHouse','CompoundBase','CompoundWing'):
        for variant in (0,1):
            ROOF=kind in ('House','Top','CompoundHouse','CompoundWing');building(kind,variant)
            export('SM_Settlement_'+kind+'_'+str(variant))
    # An asymmetric probe lets the UE importer verify XY handedness explicitly.
    box(1,1.2,2,2.2,3,3.2);export('SM_Settlement_AxisProbe')
    (OUT/'kit_manifest.json').write_text(json.dumps({'authored_for':'SpiritWFC settlement v2',
        'units':'meters','cell_size_cm':[600,600,360],'palette':COLORS,
        'front':'UE -Y; inverse Y baked before FBX conversion','fbx_y_mirrored':True,'models':MANIFEST},indent=2),encoding='utf-8')
    print('SETTLEMENT_KIT',len(MANIFEST))
