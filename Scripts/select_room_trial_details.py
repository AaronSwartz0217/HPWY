import json,re
from pathlib import Path
r=json.loads(Path('Saved/RestartRoomInventory.json').read_text(encoding='utf-8'))
sel=[]
for a in r:
 c=a['center'];e=a['extent'];label=a['label']
 if a['class']!='StaticMeshActor':continue
 if not (re.search(r'家具|cha|茶|圆环|^立方体_',label)):continue
 if all(c[i]-e[i]>=lo and c[i]+e[i]<=hi for i,lo,hi in [(0,530,1525),(1,-550,1260),(2,-120,200)]):sel.append(a)
print('count',len(sel))
print(', '.join(a['label'] for a in sel))
Path('Saved/RoomVisibilityTrial/interior_candidates.json').write_text(json.dumps(sel,ensure_ascii=False,indent=2),encoding='utf-8')
