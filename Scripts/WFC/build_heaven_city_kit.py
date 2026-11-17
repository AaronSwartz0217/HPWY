"""Build the Heaven city kit using the settlement door and stair geometry."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_settlement_kit as kit

kit.OUT = kit.ROOT / 'SourceArt/ProceduralGeneration/WFC/HeavenCity'
kit.OUT.mkdir(parents=True, exist_ok=True)
bronze = kit.bpy.data.materials.new('WFC_Palette_6')
bronze.diffuse_color = (.30, .17, .06, 1)
kit.MATS.append(bronze)
box = kit.box


def hips(lower, upper, low_z, high_z, slot=4, ridge=False):
    """Sloped, closed roof tier, with an optional long ridge."""
    def corners(x, y, z):
        return [(-x, -y, z), (x, -y, z), (x, y, z), (-x, y, z)]
    n = len(kit.VERTS)
    kit.VERTS.extend(corners(*lower, low_z) + corners(*upper, high_z))
    for i in range(4):
        j = (i + 1) % 4
        kit.FACES.append((n+i, n+j, n+j+4, n+i+4))
        kit.SLOTS.append(slot)
    kit.FACES.extend([(n+3, n+2, n+1, n), (n+4, n+5, n+6, n+7)])
    kit.SLOTS.extend([3, slot])
    if ridge:
        box(-upper[0]-.12, upper[0]+.12, -.095, .095, high_z-.04, high_z+.10, 6)


def shell(role):
    # Reuse the original floor opening, front door, window and column layout.
    # Reuse the short wall height too, but author a new roof in its place.
    pitched = kit.roof
    kit.ROOF = True
    kit.roof = lambda **_: None
    try:
        kit.building(role, 0)
    finally:
        kit.roof = pitched
    box(-2.42, 2.42, -2.42, 2.42, 2.93, 3.09, 3)


def brackets():
    for x in (-2.25, -.85, .9, 2.25):
        box(x-.10, x+.10, -2.60, -2.24, 2.71, 3.06, 3)
        box(x-.17, x+.17, -2.66, -2.46, 2.92, 3.08, 6)


def plate(x, y=-2.41, z=2.71):
    box(x-.40, x+.40, y-.04, y+.10, z-.15, z+.15, 3)
    box(x-.34, x+.34, y-.055, y-.037, z-.10, z+.10, 6)
    for offset in (-.18, 0, .18):
        box(x+offset-.026, x+offset+.026, y-.064, y-.054, z-.065, z+.065, 3)


shell('House')
brackets()
hips((2.79, 2.75), (1.5, .08), 3.06, 4.12, ridge=True)
box(-2.47, 2.47, -2.48, -2.36, 2.54, 2.65, 3)
plate(1.3)
kit.export('SM_Heaven_House_Archive')

shell('House')
# A long, low storefront canopy on the left, clear of the right-side doorway.
box(-2.71, .48, -2.93, -2.25, 2.31, 2.44, 3)
box(-2.73, .5, -2.94, -2.23, 2.44, 2.52, 4)
for x in (-2.60, .38):
    box(x-.065, x+.065, -2.9, -2.76, 0, 2.31, 3)
box(-2.62, .42, -2.87, -2.42, .82, .94, 3)
hips((2.65, 2.67), (1.74, .30), 3.06, 3.71)
plate(-1.03, z=2.70)
kit.export('SM_Heaven_House_Workshop')

shell('House')
brackets()
hips((2.86, 2.84), (1.14, 1.14), 3.06, 3.86)
box(-1.12, 1.12, -1.12, 1.12, 3.84, 4.34, 2)
hips((1.38, 1.38), (.12, .12), 4.29, 5.19)
box(-.16, .16, -.16, .16, 5.16, 5.44, 6)
plate(1.3)
kit.export('SM_Heaven_House_Observatory')

shell('House')
hips((2.48, 2.48), (2.28, 2.28), 3.05, 3.33, 5)
for x in (-2.22, 2.22):
    for y in (-2.22, 2.22):
        box(x-.10, x+.10, y-.10, y+.10, 3.33, 4.01, 3)
for y in (-2.22, 2.22):
    box(-2.25, 2.25, y-.07, y+.07, 3.73, 3.87, 6)
for x in (-2.22, 2.22):
    box(x-.07, x+.07, -2.25, 2.25, 3.73, 3.87, 6)
hips((2.64, 2.64), (.56, .42), 3.98, 4.99)
plate(1.3)
kit.export('SM_Heaven_House_BellPavilion')

shell('Top')
brackets()
hips((2.86, 2.80), (1.08, .08), 3.06, 4.34, ridge=True)
kit.export('SM_Heaven_Top_GlazedHall')

shell('Top')
hips((2.85, 2.85), (1.29, 1.29), 3.06, 3.81)
box(-1.26, 1.26, -1.26, 1.26, 3.78, 4.15, 2)
hips((1.53, 1.53), (.10, .10), 4.13, 5.12)
box(-.15, .15, -.15, .15, 5.09, 5.38, 6)
kit.export('SM_Heaven_Top_DoubleEaves')

# Update existing Heaven modules in place. The shader supplies plaza joints
# without the intersecting horizontal and vertical grout-strip meshes.
kit.ground(plaza=True, grout_geometry=False)
kit.export('SM_Settlement_Plaza')
kit.ROOF = False
for role in ('Base', 'Middle', 'CompoundBase'):
    for index in (0, 1):
        kit.building(role, index)
        kit.export(f'SM_Settlement_{role}_{index}')
