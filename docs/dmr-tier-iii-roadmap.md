# DMR Tier III Trunking — Project Roadmap

Experimental implementation of DMR Tier III trunking (mobile station side) per
**ETSI TS 102 361-4**, on top of OpenGD77.

> Status: scaffolding only. The module in `firmware/source/dmrTier3/` contains
> the state machine skeleton and stubbed API — no working Tier III protocol
> logic yet. See the phased plan below.

## 1. Goal

Give OpenGD77-based radios (Baofeng DM-1801 and other NXP MK22 + AT1846S +
HR-C6000 hardware) the ability to operate as a mobile station on a DMR Tier III
trunked network: acquire the control channel, decode the beacon, register /
affiliate, request calls, and follow channel grants to traffic channels.

As far as the author knows, no complete open-source Tier III mobile-station
stack has been published — this is greenfield work.

## 2. Scope per ETSI TS 102 361-4

In scope (mobile station procedures):

- **Control channel acquisition and beacon decoding** — the trunking control
  channel transmits continuously; its beacon bursts carry the System Identity
  Code (SYScode), network parameters and channel plan. The radio must find the
  control channel, synchronize and decode the beacon.
- **Slotted-Aloha random access** — the mobile station's only way to talk back
  on the control channel: transmit short request bursts in Aloha slots with
  backoff on collision.
- **Registration / affiliation** — register the radio's individual ID with the
  system so it can receive calls.
- **Call setup / teardown state machines** — individual and talkgroup voice
  call requests, grant handling, call maintenance and clean release.
- **Channel grants and traffic-channel operation** — on grant, retune to the
  assigned traffic channel / timeslot, run the voice call, then return to the
  control channel.

Out of scope (for now): base-station / repeater side, Tier III short data and
status messaging, authentication and encryption handling (systems using them
require operator-provisioned keys), multi-site roaming.

## 3. Architecture and integration points

The HR-C6000 handles the 4FSK modem, burst timing and timeslot sync in
hardware/firmware — Tier III is **MAC-layer software** on the MK22. The new
module lives isolated in `firmware/source/dmrTier3/` (`dmrTier3.h`,
`dmrTier3.c`) and hooks into the existing DMR stack at these points:

| Hook | Existing code | Use |
|---|---|---|
| RX frame tap | `PORTC_IRQHandler()` / RX path in `firmware/source/hardware/HR-C6000.c` | Feed received control-channel frames (slot type + payload) to `dmrTier3_processRxFrame()` for beacon / grant decoding |
| State machine tick | `tick_HR_C6000()` in `firmware/source/hardware/HR-C6000.c` (runs in the `fw_hrc6000_task()` FreeRTOS task, started from `main.c`) | Call `dmrTier3_tick()` for Aloha backoff timers, registration retries, call timeouts |
| TX start | TX-start path in `tick_HR_C6000()` (`DMR_STATE_TX_START_1`, cf. `setupPcOrTGHeader()`) | Transmit slotted-Aloha request bursts |
| RF retune | `trxSetFrequency()`, `trxSetDMRColourCode()`, `trxSetDMRTimeSlot()` in `firmware/source/functions/trx.c` | Switch between control channel and granted traffic channel/timeslot |
| PDU encode/decode patterns | `firmware/source/hotspot/` (`DMRLC.c`, `DMRSlotType.c`, `DMRFullLC.c`, `DMRShortLC.c`, `DMREmbeddedData.c`, `dmrUtils.c`) | Reuse the MMDVM-derived bit-manipulation patterns for Tier III CACH / slot-type / PDU handling |
| Build wiring | `firmware/Makefile` (per-directory `$(wildcard ...)` source lists) | Phase 1: add `$(wildcard ../source/dmrTier3/*.c)` and the include path; module is compiled only when the feature flag is enabled (see section 7) |

## 4. Phased plan

- **Phase 0 — Spec mapping.** Map TS 102 361-4 trunking procedures (beacon
  structure, random access, registration, call control PDUs) to concrete
  module APIs. Deliverable: this document + the skeleton.
- **Phase 1 — Passive control-channel beacon decoder.** Tap the RX path,
  decode beacon bursts, extract SYScode and channel plan, display lock status.
  No transmission. (Validates the RX hook and the C6000's ability to track the
  control channel.)
- **Phase 2 — Slotted-Aloha random access TX.** Implement Aloha slot timing,
  backoff and request-burst transmission using the existing TX-start path.
- **Phase 3 — Registration / affiliation.** Registration request/response
  state machine; handle accept, reject and retry.
- **Phase 4 — Call setup + traffic channel.** Call request, grant parsing,
  retune to the traffic channel via `trxSetFrequency()`, voice call, return to
  control channel on release.
- **Phase 5 — Interop test.** Full attach → register → call → release cycle
  against the author's own experimental Tier III network (see section 6).

## 5. Hardware constraints

- **Modem:** HR-C6000 does 4FSK modulation/demodulation and burst handling —
  Tier III needs no modem changes, only MAC-layer state machines driving the
  existing RX/TX primitives.
- **MCU:** NXP MK22FN512VLL12 — 512 KB flash, 128 KB RAM. The Tier III module
  is small (state tables + PDU buffers); flash/RAM budget to be measured once
  Phase 1 is wired into the build.
- **Build toolchain:** MCUXpresso IDE + NXP MK22 SDK (see `BUILD.md`). Note:
  a full firmware build also requires the proprietary AMBE codec blobs
  (`linkerdata/codec_bin_section_*.bin`) extracted from the official
  Radioddity firmware 3.1.1 — without them the image cannot be linked. The
  Tier III module itself has no codec dependency.
- **Timing:** Aloha slot timing and grant-following retunes must fit the
  C6000 interrupt/tick cadence; worst-case retune latency to be measured in
  Phase 4.

## 6. Testing

Testing is limited to:

1. The author's **own experimental Tier III network** (radios + a minimal
   control-channel base station, e.g. MMDVM- or SDR-based beacon), and/or
2. Real Tier 3 systems whose **operator has explicitly authorized** testing.

No transmission toward, and no affiliation attempt with, any trunked system
without the operator's permission. Passive (receive-only) decoding with an SDR
is the recommended way to study real control channels before transmitting
anything.

## 7. Upstream contribution (possible future)

The Tier III code is deliberately kept **isolated** in `firmware/source/dmrTier3/`
with a minimal, well-defined hook surface into the existing stack
(`dmrTier3_init()`, `dmrTier3_tick()`, `dmrTier3_processRxFrame()`), and it is
compiled only when the feature flag is enabled, so the default build is
byte-identical to upstream. This keeps the door open to one day propose the
work upstream to rogerclarkmelbourne/OpenGD77 as an **opt-in feature**.

Notes:

- No CONTRIBUTING file or written style guide was found in the repository, so
  the module follows the codebase's observed conventions: C with tabs for
  indentation, `camelCase` function names, `UPPER_SNAKE_CASE` macros, English
  comments.
- As of this writing the upstream repository
  (rogerclarkmelbourne/OpenGD77) does not appear to be available on GitHub
  (API returns an error; the fork README notes it was deleted), so an upstream
  proposal is aspirational for now.
- **Per the owner's standing rule, no upstream pull request will ever be
  opened without his explicit per-case confirmation.**

## 8. References

- ETSI TS 102 361-4 — Digital Mobile Radio (DMR) Systems; Part 4: DMR trunking
  protocol.
- ETSI TS 102 361-1..3 — DMR air interface (bursts, channel coding, data
  protocol) — needed for the beacon/PDU bit layouts.
- OpenGD77 `BUILD.md` — toolchain and build instructions.
- `firmware/source/hotspot/` — existing DMR PDU encode/decode reference code.
