import csv,json,statistics
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Saved/RoomVisibilityTrial';result={}
for label in ['Baseline','Trial']:
 p=next((root/('Saved/RoomBenchmark_'+label)).rglob('Profile*.csv'))
 with p.open(encoding='utf-8-sig',newline='') as f:rows=list(csv.reader(f))
 h=rows[0];keys=['FrameTime','GPUTime','GameThreadTime','RenderThreadTime','RHI/DrawCalls'];elapsed=0;data=[]
 for row in rows[1:]:
  try:v={k:float(row[h.index(k)]) for k in keys}
  except (ValueError,IndexError):continue
  if v['FrameTime']<=0:continue
  elapsed+=v['FrameTime']/1000
  if elapsed>10:data.append(v)
 assert len(data)>100,(label,len(data))
 r={'file':str(p),'frames':len(data),'seconds':sum(v['FrameTime'] for v in data)/1000,'metrics':{}}
 for k in keys:
  values=sorted(v[k] for v in data);r['metrics'][k]={'mean':statistics.mean(values),'p95':values[int((len(values)-1)*.95)],'p99':values[int((len(values)-1)*.99)]}
 r['fps']=1000/r['metrics']['FrameTime']['mean'];r['over_16_67_percent']=100*sum(v['FrameTime']>1000/60 for v in data)/len(data)
 result[label]=r
result['conditions']={'resolution':'1920x1080 output','view':'fixed player spawn camera','warmup_removed_seconds':10,'order':'Baseline then Trial','note':'Single sequential samples, not a full traversal benchmark; copies have different map names and warm caches may affect comparison.'}
(out/'benchmark_comparison.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2))
