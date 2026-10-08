# Cloud Hosting Costs

Target: GPU VM with an NVIDIA NVENC encoder for Pixel Streaming: AWS `g4dn.xlarge` (T4 GPU), Windows, region `ap-south-1` (Mumbai).

## Day 1 estimate
_Prices checked on: 2026-09-25. Compute prices checked; other values are estimates (marked ~)._

| Item | Price | Source |
|------|-------|--------|
| g4dn.xlarge Windows on-demand | ~~$0.526 / h~~ ~$0.76 / h | **Wrong in this table:** $0.526 is the US Linux rate. Mumbai Linux is $0.579 / h (rechecked 2026-10-08); Windows adds a license fee, ~$0.76 / h is an estimate. Totals below are ~30% low |
| g4dn.xlarge Windows spot | $0.2366 / h | Checked |
| EBS gp3 storage, 100 GB | ~$0.091 / GB-month → ~$9.12 / month | Estimate; billed even while the VM is stopped |
| Public IPv4 address | ~$0.005 / h while running | Estimate |
| Data transfer out | ~$0.109 / GB after ~100 GB/month free | Estimate |
| Stream bandwidth | ~10 Mbps ≈ 4.5 GB per viewer-hour | Estimate |

### Monthly totals

Assumes every running hour has one viewer watching (upper bound for transfer): 60 viewer-hours → 270 GB → 170 GB billable ≈ $18.58.

| Scenario | Hours/month | Compute | Storage | IPv4 | Transfer | **Total** |
|----------|-------------|---------|---------|------|----------|-----------|
| 2 h/day, on-demand | 60 | $31.56 | $9.12 | $0.30 | $18.58 | **~$60** |
| 2 h/day, spot | 60 | $14.20 | $9.12 | $0.30 | $18.58 | **~$42** |
| Always on, on-demand | 730 | $383.98 | $9.12 | $3.65 | $18.58 | **~$415** |
| Always on, spot | 730 | $172.72 | $9.12 | $3.65 | $18.58 | **~$204** |

(Always-on rows assume the same 60 viewer-hours; the VM idles the rest of the time.)

### Per demo session
One 30-minute session on-demand (incl. ~5 min boot): ~$0.26 compute + ~$0.25 transfer ≈ **~$0.50**.
Fixed floor regardless of use: ~$9 / month storage.

**Decision (2026-09-25): $0 path, no cloud hosting.**
Pixel Streaming runs locally only; the demo video is the portfolio asset and live demos are run from my laptop
(screen share in interviews). Day 14 becomes a deployment design (below) instead of a deployment.

If deployed later: on-demand, not spot (spot can be reclaimed mid-session), auto-shutdown when idle, start on demand.
Always-on costs ~7× more than 2 h/day for the same viewing.

## Recheck 2026-10-08 (one-off test session and monthly hosting)
_Sources: third-party price lists for ap-south-1 (official Mumbai Windows and egress rates not found), AWS IPv4 announcement, AWS India
billing docs. Confirm in the AWS Pricing Calculator (region Mumbai, OS Windows) before launching anything._

**One 4 h test session, everything deleted the same day:**

| Item | When charged | Estimate |
|------|--------------|----------|
| Card check at signup | AWS India charges ₹2, refunded after verification | ₹2, refunded |
| g4dn.xlarge Windows on-demand | per second while running | 4 h × ~$0.76 ≈ $3.04 |
| 100 GB gp3 ($0.0912 / GB-month) | while the volume exists, also when the VM is stopped | ≈ $0.30 for a day |
| Public IPv4 | $0.005 / h | ≈ $0.02 |
| Data out (~4.5 GB per viewer-hour) | first 100 GB / month free, then ~₹8–9 / GB (reseller figure) | $0 |
| Upload of the packaged build | inbound is free | $0 |
| vCPU quota request, budget alert | free | $0 |
| GST 18% | if billed by AWS India (Indian address) | ≈ +$0.60 |
| **Total** | | **≈ $3.50–4.50** (Linux instead of Windows: ≈ $2.70) |

**Monthly hosting, corrected:** 2 h/day on-demand ≈ $74 (≈ $87 with GST); always on ≈ $586 (≈ $690 with GST). Floor ≈ $9 / month
disk even when stopped.

**Traps:** a forgotten VM ≈ $18 / day; stopping alone still bills the disk; budget alerts only email, they never cap the bill; new
accounts (since July 2025) get $100–200 credits, but the Free plan restricts high-consumption services (GPU likely, not confirmed),
so g4dn may need the Paid plan, where use beyond the credits goes to the card; snapshots / AMIs kept for later are billed per GB-month;
a USD-billed account can add a bank forex markup (~3.5% + GST on it).

**Decision unchanged (2026-10-08): $0.** The portfolio uses the demo video and live screen-share; Day 14 stays a design.

## Day 14: deployment design (not deployed)
_TODO (Day 14):_
- VM: g4dn.xlarge Windows, ap-south-1, 100 GB gp3
- Ports / security group: signalling (HTTP/WS), WebRTC UDP range, TURN
- TURN: coturn on the same VM; when it is needed (strict NAT, mobile data)
- Auto-shutdown: script that stops the VM after N minutes with no connected viewer
- Start on demand: small cloud function + "warming up, about 2–3 minutes" page
- Relay runs on the same VM so the latency clock stays valid

## Future option: one-off deployment (~$2–5)
1. Create an AWS Budget alert at $5 (free) before launching anything.
2. Check the G-instance vCPU quota in ap-south-1; request ≥ 4 if it is 0.
3. Launch on-demand for one 3–4 h session; test from mobile data, TURN and auto-shutdown.
4. Record real cost per hour and per session here.
5. **Terminate** the VM and delete the EBS volume (stopping alone still bills storage).
