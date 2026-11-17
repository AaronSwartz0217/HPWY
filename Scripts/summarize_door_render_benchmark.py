import csv,json,statistics
from pathlib import Path
root=Path(__file__).resolve().parents[1];folder=root/'Saved/DoorRenderBenchmark_20261103'
p=next(folder.rglob('Profile*.csv'))
with p.open(encoding='utf-8-sig',newline='') as f:rows=list(csv.reader(f))
h=rows[0];keys=['FrameTime','GPUTime','GameThreadTime','RenderThreadTime','RHI/DrawCalls'];data=[];transitions=[]
for row in rows[1:]:
 try:v={k:float(row[h.index(k)]) for k in keys}
 except (ValueError,IndexError):continue
 if v['FrameTime']<=0:continue
 event=row[0];i=len(data)
 if 'DOOR_RENDER_OPT_ON' in event:transitions.append((i,'on'))
 if 'DOOR_RENDER_OPT_OFF' in event:transitions.append((i,'off'))
 data.append(v)
assert len(data)>2500,(len(data),transitions)
ons=[i for i,s in transitions if s=='on'];assert ons,transitions
on=ons[0];offs=[i for i,s in transitions if s=='off' and i>on];assert offs,transitions
off=offs[0]
result={'csv':str(p),'conditions':{'map':'场景优化','output_resolution':'1920x1080','screen_percentage':100,'view':'fixed outdoor spawn camera, three doors closed','same_process_order':'off/on/off','phase_trim':'discard first250 and last50 frames of each phase','transitions':transitions},'phases':{}}
for name,start,end in [('off_before',0,on),('on',on,off),('off_after',off,len(data))]:
 sample=data[start+250:end-50];assert len(sample)>300,(name,len(sample))
 r={'frames':len(sample),'seconds':sum(x['FrameTime'] for x in sample)/1000,'metrics':{}}
 for k in keys:
  values=sorted(x[k] for x in sample);r['metrics'][k]={'mean':statistics.mean(values),'p50':statistics.median(values),'p95':values[int((len(values)-1)*.95)],'p99':values[int((len(values)-1)*.99)]}
 r['fps']=1000/r['metrics']['FrameTime']['mean'];r['over_16_67_percent']=100*sum(x['FrameTime']>1000/60 for x in sample)/len(sample)
 result['phases'][name]=r
before=result['phases']['off_before']['metrics']['FrameTime']['mean'];after=result['phases']['off_after']['metrics']['FrameTime']['mean'];enabled=result['phases']['on']['metrics']['FrameTime']['mean']
result['saved_ms_vs_warm_off']=after-enabled;result['reduction_percent_vs_warm_off']=100*(after-enabled)/after;result['baseline_drift_percent']=100*abs(after-before)/before
(folder/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2))
