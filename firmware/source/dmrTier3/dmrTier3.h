/*
 * dmrTier3.h
 *
 * Experimental DMR Tier III trunking (mobile station) support per ETSI TS 102 361-4.
 *
 * Status: SCAFFOLDING ONLY (Phase 0). State machine states and the public API
 * are defined; no protocol logic is implemented yet. See docs/dmr-tier-iii-roadmap.md.
 *
 * Integration:
 *   - Call dmrTier3_init() once at startup (after HRC6000_init()).
 *   - Call dmrTier3_tick() from tick_HR_C6000() (firmware/source/hardware/HR-C6000.c).
 *   - Feed received control-channel frames via dmrTier3_processRxFrame() from the
 *     RX path in firmware/source/hardware/HR-C6000.c.
 *   - The module retunes via trxSetFrequency() (firmware/source/functions/trx.c).
 *
 * The module is compiled only when ENABLE_DMR_TIER3 is defined, so the default
 * build stays byte-identical to upstream.
 */

#ifndef DMR_TIER3_H
#define DMR_TIER3_H

#include <stdint.h>
#include <stdbool.h>

/* --- Mobile station state machine states (TS 102 361-4 trunking procedures) --- */
typedef enum
{
	TIER3_STATE_OFF = 0,          /* Module disabled / not started */
	TIER3_STATE_BEACON_ACQUIRE,   /* Scanning for the trunking control channel */
	TIER3_STATE_BEACON_LOCKED,    /* Beacon decoded (SYScode known), monitoring control channel */
	TIER3_STATE_REGISTERING,      /* Slotted-Aloha registration request in progress */
	TIER3_STATE_IDLE_REGISTERED,  /* Affiliated; idle on the control channel */
	TIER3_STATE_CALL_SETUP,       /* Call requested; awaiting channel grant */
	TIER3_STATE_TRAFFIC,          /* Active call on the granted traffic channel/timeslot */
	TIER3_STATE_DEREGISTERING     /* Deregistration in progress */
} dmrTier3State_t;

/* --- Decoded beacon / system information (from the control channel) --- */
typedef struct
{
	uint32_t syscode;             /* System Identity Code broadcast in the beacon */
	uint32_t controlChannelHz;    /* Control channel frequency in Hz */
	uint8_t  colourCode;          /* DMR colour code of the system */
	bool     valid;               /* True once a beacon has been decoded (Phase 1) */
} dmrTier3SystemInfo_t;

/* --- Channel grant (parsed from a grant PDU on the control channel) --- */
typedef struct
{
	uint32_t trafficChannelHz;    /* Assigned traffic channel frequency in Hz */
	uint8_t  timeslot;            /* Assigned timeslot (1 or 2) */
	uint32_t talkgroupOrId;       /* Talkgroup or individual ID the grant is for */
	bool     valid;
} dmrTier3Grant_t;

/*
 * Initialise the Tier III module. Resets state to TIER3_STATE_OFF.
 * Call once at startup, after HRC6000_init().
 */
void dmrTier3_init(void);

/*
 * Start trunking operation on the given control channel frequency.
 * Moves the state machine to TIER3_STATE_BEACON_ACQUIRE.
 */
void dmrTier3_start(uint32_t controlChannelHz);

/*
 * Stop trunking operation. Returns the radio to normal (Tier II) operation.
 */
void dmrTier3_stop(void);

/*
 * Periodic tick. Handles Aloha backoff timers, registration retries and call
 * timeouts. Intended to be called from tick_HR_C6000().
 * WIP (Phase 0): state machine transitions not implemented yet.
 */
void dmrTier3_tick(void);

/*
 * Feed one received frame from the control channel into the module.
 *   frame    : pointer to the decoded frame payload bytes.
 *   frameLen : payload length in bytes.
 *   slotType : DMR slot type of the burst (to distinguish beacon / CACH / PDU).
 * WIP (Phase 1): beacon decoding not implemented yet.
 */
void dmrTier3_processRxFrame(const uint8_t *frame, uint8_t frameLen, uint8_t slotType);

/*
 * Request registration (affiliation) with the trunked system.
 * Uses slotted-Aloha random access on the control channel.
 * WIP (Phase 3): not implemented yet.
 */
void dmrTier3_register(void);

/*
 * Request deregistration from the system.
 * WIP (Phase 3): not implemented yet.
 */
void dmrTier3_deregister(void);

/*
 * Request a voice call to a talkgroup or individual ID.
 * On grant, the module retunes to the traffic channel (Phase 4).
 * WIP (Phase 4): not implemented yet.
 */
void dmrTier3_requestCall(uint32_t talkgroupOrId, bool isGroupCall);

/*
 * End the current call and return to the control channel.
 * WIP (Phase 4): not implemented yet.
 */
void dmrTier3_endCall(void);

/*
 * Accessors.
 */
dmrTier3State_t dmrTier3_getState(void);
const dmrTier3SystemInfo_t *dmrTier3_getSystemInfo(void);
const dmrTier3Grant_t *dmrTier3_getActiveGrant(void);

#endif /* DMR_TIER3_H */
