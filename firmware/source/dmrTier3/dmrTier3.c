/*
 * dmrTier3.c
 *
 * Experimental DMR Tier III trunking (mobile station) support per ETSI TS 102 361-4.
 *
 * Status: SCAFFOLDING ONLY (Phase 0). All functions are stubs; the state
 * machine transitions, beacon decoder, Aloha random access, registration and
 * call control are NOT implemented yet. See docs/dmr-tier-iii-roadmap.md for
 * the phased plan.
 *
 * NOTE: this file is intentionally NOT wired into firmware/Makefile yet.
 * Phase 1 will add a wildcard for ../source/dmrTier3 sources
 * to the SOURCES list, guarded by the ENABLE_DMR_TIER3 feature flag.
 */

#include "dmrTier3.h"

/* --- Module state --- */
static dmrTier3State_t tier3State = TIER3_STATE_OFF;
static dmrTier3SystemInfo_t tier3SystemInfo = { 0, 0, 0, false };
static dmrTier3Grant_t tier3ActiveGrant = { 0, 0, 0, false };

void dmrTier3_init(void)
{
	tier3State = TIER3_STATE_OFF;
	tier3SystemInfo.valid = false;
	tier3ActiveGrant.valid = false;
}

void dmrTier3_start(uint32_t controlChannelHz)
{
	/* WIP (Phase 1): tune the receiver to the control channel via
	 * trxSetFrequency() and enter beacon acquisition. */
	(void)controlChannelHz;
	tier3State = TIER3_STATE_BEACON_ACQUIRE;
}

void dmrTier3_stop(void)
{
	/* WIP: deregister if affiliated, then restore normal Tier II operation. */
	tier3State = TIER3_STATE_OFF;
	tier3ActiveGrant.valid = false;
}

void dmrTier3_tick(void)
{
	/* WIP (Phase 0): state machine transitions not implemented yet.
	 * Future work per TS 102 361-4 trunking procedures:
	 *   - TIER3_STATE_BEACON_ACQUIRE: beacon search timeout / channel scan.
	 *   - TIER3_STATE_REGISTERING:   Aloha backoff timer, registration retry.
	 *   - TIER3_STATE_CALL_SETUP:    grant wait timeout.
	 *   - TIER3_STATE_TRAFFIC:       call hang-time / release detection.
	 */
	switch (tier3State)
	{
		case TIER3_STATE_OFF:
		case TIER3_STATE_BEACON_ACQUIRE:
		case TIER3_STATE_BEACON_LOCKED:
		case TIER3_STATE_REGISTERING:
		case TIER3_STATE_IDLE_REGISTERED:
		case TIER3_STATE_CALL_SETUP:
		case TIER3_STATE_TRAFFIC:
		case TIER3_STATE_DEREGISTERING:
		default:
			break;
	}
}

void dmrTier3_processRxFrame(const uint8_t *frame, uint8_t frameLen, uint8_t slotType)
{
	/* WIP (Phase 1): decode the control channel beacon here.
	 * Per TS 102 361-4, the beacon carries the System Identity Code (SYScode);
	 * on successful decode, fill tier3SystemInfo and move to
	 * TIER3_STATE_BEACON_LOCKED. Later phases also parse channel grants into
	 * tier3ActiveGrant and move TIER3_STATE_CALL_SETUP -> TIER3_STATE_TRAFFIC. */
	(void)frame;
	(void)frameLen;
	(void)slotType;
}

void dmrTier3_register(void)
{
	/* WIP (Phase 3): build the registration PDU and transmit it using
	 * slotted-Aloha random access (Phase 2) on the control channel. */
	if (tier3State == TIER3_STATE_BEACON_LOCKED)
	{
		tier3State = TIER3_STATE_REGISTERING;
	}
}

void dmrTier3_deregister(void)
{
	/* WIP (Phase 3): not implemented yet. */
	if (tier3State == TIER3_STATE_IDLE_REGISTERED)
	{
		tier3State = TIER3_STATE_DEREGISTERING;
	}
}

void dmrTier3_requestCall(uint32_t talkgroupOrId, bool isGroupCall)
{
	/* WIP (Phase 4): send the call request PDU via random access; on grant,
	 * retune with trxSetFrequency() to tier3ActiveGrant.trafficChannelHz and
	 * enter TIER3_STATE_TRAFFIC. */
	(void)talkgroupOrId;
	(void)isGroupCall;
	if (tier3State == TIER3_STATE_IDLE_REGISTERED)
	{
		tier3State = TIER3_STATE_CALL_SETUP;
	}
}

void dmrTier3_endCall(void)
{
	/* WIP (Phase 4): release the call, retune to the control channel and
	 * return to TIER3_STATE_IDLE_REGISTERED. */
	if (tier3State == TIER3_STATE_TRAFFIC)
	{
		tier3ActiveGrant.valid = false;
		tier3State = TIER3_STATE_IDLE_REGISTERED;
	}
}

dmrTier3State_t dmrTier3_getState(void)
{
	return tier3State;
}

const dmrTier3SystemInfo_t *dmrTier3_getSystemInfo(void)
{
	return &tier3SystemInfo;
}

const dmrTier3Grant_t *dmrTier3_getActiveGrant(void)
{
	return &tier3ActiveGrant;
}
