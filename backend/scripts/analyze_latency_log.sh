#!/usr/bin/env bash
set -euo pipefail

LOG="${1:-/tmp/cortex_latency2.log}"

if [[ ! -f "$LOG" ]]; then
  echo "log_not_found=$LOG"
  exit 1
fi

echo "--- publish aggregates ---"
awk '
match($0,/publish_timing .*mutex_wait_ms=([0-9]+).*xadd_ms=([0-9]+).*total_ms=([0-9]+)/,a){
  n++;
  mw=a[1]+0;
  xa=a[2]+0;
  tt=a[3]+0;
  smw+=mw;
  sxa+=xa;
  stt+=tt;
  if(mw>maxmw) maxmw=mw;
  if(xa>maxxa) maxxa=xa;
  if(tt>maxtt) maxtt=tt;
}
END{
  if(n>0){
    printf("count=%d avg_mutex_wait_ms=%.2f max_mutex_wait_ms=%d avg_xadd_ms=%.2f max_xadd_ms=%d avg_total_ms=%.2f max_total_ms=%d\n", n, smw/n, maxmw, sxa/n, maxxa, stt/n, maxtt);
  } else {
    print "count=0";
  }
}' "$LOG"

echo "--- submit aggregates ---"
awk '
match($0,/submit_timing .*mysql_save_ms=([0-9]+)/,a){n1++;v=a[1]+0;s1+=v;if(v>m1)m1=v}
match($0,/submit_timing .*redis_publish_ms=([0-9]+)/,b){n2++;v=b[1]+0;s2+=v;if(v>m2)m2=v}
match($0,/submit_timing .*total_ms=([0-9]+)/,c){n3++;v=c[1]+0;s3+=v;if(v>m3)m3=v}
END{
  if(n1>0) printf("mysql_save count=%d avg=%.2f max=%d\n", n1, s1/n1, m1); else print "mysql_save count=0";
  if(n2>0) printf("redis_publish count=%d avg=%.2f max=%d\n", n2, s2/n2, m2); else print "redis_publish count=0";
  if(n3>0) printf("submit_total count=%d avg=%.2f max=%d\n", n3, s3/n3, m3); else print "submit_total count=0";
}' "$LOG"

echo "--- backpressure summary ---"
awk '
/index=\"jobs.submit.backpressure\"/{bp2029++}
/status=backpressure/{bpstatus++}
/backpressure_timing/{bptime++}
END{
  printf("backpressure_logs=%d status_backpressure_logs=%d backpressure_timing_logs=%d\n", bp2029+0, bpstatus+0, bptime+0);
}' "$LOG"
