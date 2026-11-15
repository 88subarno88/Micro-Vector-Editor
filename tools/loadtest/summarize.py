import sys,json
for l in sys.stdin:
  if l.startswith('{'):
    r=json.loads(l); a=r['ackLatencyMs']
    print(f"  offered {r['offeredOpsPerSec']}/s  acked {r['ackedOpsPerSec']}/s  rejects {r['rejects']}  ack p50/p99 {a['p50']}/{a['p99']} ms  fanout p50/p99 {r['fanoutLatencyMs']['p50']}/{r['fanoutLatencyMs']['p99']}  recv {r['receivedPerSec']}  closed {r['closedEarly']}  gen {r['loadGenCpuPct']}%  joinP50 {r['setup']['joinP50Ms']}ms")
  else: print(l,end='')
