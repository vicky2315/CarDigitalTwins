# Cloud Hosting Costs

Target: GPU VM with an NVIDIA NVENC encoder for Pixel Streaming (e.g. AWS `g4dn.xlarge`, T4 GPU), region nearest to me (ap-south-1 Mumbai if available).

## Day 1 estimate
_Prices checked on: TODO_

| Instance | Region | On-demand $/h | Spot $/h | Notes |
|----------|--------|---------------|----------|-------|
| g4dn.xlarge | ap-south-1 | TODO | TODO | |

| Scenario | Hours/month | On-demand $/month | Spot $/month |
|----------|-------------|-------------------|--------------|
| 2 h/day | ~60 | TODO | TODO |
| Always on | ~730 | TODO | TODO |

Also count: storage (EBS), public IP, data transfer out.

**Decision:** _TODO: on-demand for demos (spot can be reclaimed mid-session), auto-shutdown, start on demand._

## Day 14 actuals
_TODO: real cost per hour and per demo session._
