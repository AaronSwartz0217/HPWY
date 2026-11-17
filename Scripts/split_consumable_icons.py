"""User-requested sprite-sheet slicing: preserve RGBA; output five 128px PNGs."""
from PIL import Image
from pathlib import Path
import argparse,json
parser=argparse.ArgumentParser();parser.add_argument('atlas');args=parser.parse_args()
root=Path(__file__).resolve().parents[1]/'SourceArt/UI/Items'
root.mkdir(parents=True,exist_ok=True)
image=Image.open(args.atlas).convert('RGBA')
assert image.width*2==image.height*3,'Expected a 3x2 atlas'
image.save(root/'ConsumablesAtlas_Glazed.png')
names=['XP_Small','XP_Medium','XP_Large','CooldownHaste','CooldownReset']
width,height=image.width//3,image.height//2
for i,name in enumerate(names):
    x,y=(i%3)*width,(i//3)*height
    icon=image.crop((x,y,x+width,y+height)).resize((128,128),Image.Resampling.LANCZOS)
    icon.save(root/f'T_Item_{name}.png')
(root/'manifest.json').write_text(json.dumps({'source':'ConsumablesAtlas_Glazed.png','grid':[3,2],'size':[128,128],'icons':names,'style':'simple glazed ceramic with subtle highlight'},ensure_ascii=False,indent=2),encoding='utf-8')
print('Sliced five 128x128 RGBA icons:',root)
