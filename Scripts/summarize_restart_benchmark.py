import csv,json,statistics
from pathlib import Path
root=Path(__file__).resolve().parents[1];folder=root/'Saved/Performance60FPS_20261102'
def summarize(path):
 with path.open(encoding='utf-8-sig',newline='') as f:rows=list(csv.reader(f))
 h=rows[0];keys=['FrameTime','GameThreadTime','RenderThreadTime','RHIThreadTime','GPUTime','RenderThreadTime_CriticalPath','GameThreadTime_CriticalPath','RHI/DrawCalls','RHI/PrimitivesDrawn']
 keys=[k for k in keys if k in h];data=[];elapsed=0
 for row in rows[1:]:
  try:v={k:float(row[h.index(k)]) for k in keys}
  except (IndexError,ValueError):continue
  if v['FrameTime']<=0:continue
  elapsed+=v['FrameTime']/1000
  if elapsed>10:data.append(v)
 out={'file':str(path),'warmup_excluded_seconds':10,'sample_frames':len(data),'sample_seconds':sum(v['FrameTime'] for v in data)/1000,'metrics':{}}
 for k in keys:
  vals=sorted(v[k] for v in data);out['metrics'][k]={'mean':statistics.mean(vals),'median':statistics.median(vals),'p95':vals[int((len(vals)-1)*.95)],'p99':vals[int((len(vals)-1)*.99)]}
 times=[v['FrameTime'] for v in data];out['fps']=1000/statistics.mean(times);out['frames_over_16_67_percent']=100*sum(t>1000/60 for t in times)/len(times);out['frames_over_33_33_percent']=100*sum(t>1000/30 for t in times)/len(times)
 return out
files=sorted(folder.rglob('Profile*.csv'),key=lambda p:p.stat().st_mtime)
results={label:summarize(p) for label,p in zip(['baseline','shadows_disabled'],files)}
(folder/'benchmark_comparison.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
for label,r in results.items():
 print(label,json.dumps(r,ensure_ascii=False))
