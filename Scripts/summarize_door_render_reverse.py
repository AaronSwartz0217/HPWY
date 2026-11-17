import csv,json,statistics,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];folder=root/'Saved/DoorRenderBenchmark_20261103_Reverse';p=next(folder.rglob('Profile*.csv'))
with p.open(encoding='utf-8-sig',newline='') as f:rows=list(csv.reader(f))
h=rows[0];keys=['FrameTime','GPUTime','GameThreadTime','RenderThreadTime','RHI/DrawCalls'];data=[];events=[]
for row in rows[1:]:
 try:v={k:float(row[h.index(k)]) for k in keys}
 except (ValueError,IndexError):continue
 if v['FrameTime']<=0:continue
 if 'DOOR_RENDER_OPT_OFF' in row[0]:events.append((len(data),'off'))
 if 'DOOR_RENDER_OPT_ON' in row[0]:events.append((len(data),'on'))
 data.append(v)
assert len(data)>2500,(len(data),events)
off=next(i for i,s in events if s=='off');on=next(i for i,s in events if s=='on' and i>off)
r={'csv':str(p),'order':'on/off/on','discard_each_phase':'first250 and last50 frames','phases':{}}
for name,start,end in [('on_before',0,off),('off',off,on),('on_after',on,len(data))]:
 sample=data[start+250:end-50];vals={k:sorted(v[k] for v in sample) for k in keys}
 stats={k:{'mean':statistics.mean(a),'p95':a[int((len(a)-1)*.95)],'p99':a[int((len(a)-1)*.99)]} for k,a in vals.items()}
 r['phases'][name]={'frames':len(sample),'metrics':stats,'fps':1000/stats['FrameTime']['mean'],'over_16_67_percent':100*sum(v['FrameTime']>1000/60 for v in sample)/len(sample)}
(folder/'summary.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(r,ensure_ascii=False,indent=2))
