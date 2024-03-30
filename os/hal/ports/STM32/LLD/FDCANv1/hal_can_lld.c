/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    FDCANv1/hal_can_lld.c
 * @brief   STM32 CAN subsystem low level driver source.
 *
 * @addtogroup CAN
 * @{
 */

#include "hal.h"

#if HAL_USE_CAN || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* Value of instance */
#if (STM32_CAN_USE_FDCAN1 && STM32_CAN_USE_FDCAN2 && STM32_CAN_USE_FDCAN3)
#define CAN1_OFFSET_INSTANCE            (0U) // CAN1 DEFINED
#define CAN2_OFFSET_INSTANCE            (1U) // CAN2 DEFINED
#define CAN3_OFFSET_INSTANCE            (2U) // CAN3 DEFINED
#elif (STM32_CAN_USE_FDCAN1 && STM32_CAN_USE_FDCAN2 && !STM32_CAN_USE_FDCAN3)
#define CAN1_OFFSET_INSTANCE            (0U) // CAN1 DEFINED
#define CAN2_OFFSET_INSTANCE            (1U) // CAN2 DEFINED
#elif (STM32_CAN_USE_FDCAN1 && STM32_CAN_USE_FDCAN3 && !STM32_CAN_USE_FDCAN2)
#define CAN1_OFFSET_INSTANCE            (0U) // CAN1 DEFINED
#define CAN3_OFFSET_INSTANCE            (1U) // CAN3 DEFINED
#elif (STM32_CAN_USE_FDCAN2 && STM32_CAN_USE_FDCAN3 && !STM32_CAN_USE_FDCAN1)
#define CAN2_OFFSET_INSTANCE            (0U) // CAN2 DEFINED
#define CAN3_OFFSET_INSTANCE            (1U) // CAN3 DEFINED
#elif (STM32_CAN_USE_FDCAN1 && !STM32_CAN_USE_FDCAN2 && !STM32_CAN_USE_FDCAN3)
#define CAN1_OFFSET_INSTANCE            (0U) // CAN1 DEFINED
#elif (STM32_CAN_USE_FDCAN2 && !STM32_CAN_USE_FDCAN1 && !STM32_CAN_USE_FDCAN3)
#define CAN2_OFFSET_INSTANCE            (0U) // CAN2 DEFINED
#elif (STM32_CAN_USE_FDCAN3 && !STM32_CAN_USE_FDCAN1 && !STM32_CAN_USE_FDCAN2)
#define CAN3_OFFSET_INSTANCE            (0U) // CAN3 DEFINED
#endif

/* Filter Standard Element MAX Size in WORDS for single element. */
#define SRAMCAN_FLS_SIZE                (1U)

/* Filter Extended Element MAX Size in WORDS for single element.*/
#define SRAMCAN_FLE_SIZE                (2U)

/* RX FIFO 0 Elements MAX Size in WORDS for single element.*/
#define SRAMCAN_RF0_SIZE                (18U)

/* RX FIFO 1 Elements MAX Size in WORDS for single element.*/
#define SRAMCAN_RF1_SIZE                (18U)

/* RX Buffer MAX Size in WORDS for single element.*/
#define SRAMCAN_RB_SIZE                 (18U)

/* TX Event FIFO Elements MAX Size in WORDS for single element.*/
#define SRAMCAN_TEF_SIZE                (2U)

/* TX FIFO/Queue Elements MAX Size in WORDS for single element.*/
#define SRAMCAN_TB_SIZE                 (18U)

/* Trigger Memory MAX Size in WORDS for single element.*/
#define SRAMCAN_TM_SIZE                 (2U)

/* Filter List Standard Offset.*/
#define SRAMCAN_FLSSA ((uint32_t)0)

/* Filter List Extended Offset.*/
#define SRAMCAN_FLESA ((uint32_t)(SRAMCAN_FLSSA +                           \
                                  (STM32_FDCAN_FLS_NBR * SRAMCAN_FLS_SIZE)))

/* RX FIFO 0 Offset.*/
#define SRAMCAN_RF0SA ((uint32_t)(SRAMCAN_FLESA +                           \
                                  (STM32_FDCAN_FLE_NBR * SRAMCAN_FLE_SIZE)))

/* RX FIFO 1 Offset.*/
#define SRAMCAN_RF1SA ((uint32_t)(SRAMCAN_RF0SA +                           \
                                  (STM32_FDCAN_RF0_NBR * SRAMCAN_RF0_SIZE)))

/* RX Buffer Offset.*/
#define SRAMCAN_RBSA  ((uint32_t)(SRAMCAN_RF1SA +                           \
                                  (STM32_FDCAN_RF1_NBR * SRAMCAN_RF1_SIZE)))

/* TX Event FIFO Offset.*/
#define SRAMCAN_TEFSA ((uint32_t)(SRAMCAN_RBSA +                            \
                                  (STM32_FDCAN_RB_NBR * SRAMCAN_RB_SIZE)))

/* TX Buffers Offset.*/
#define SRAMCAN_TBSA  ((uint32_t)(SRAMCAN_TEFSA +                           \
                                  (STM32_FDCAN_TEF_NBR * SRAMCAN_TEF_SIZE)))

/* Trigger Memory Offset.*/
#define SRAMCAN_TMSA  ((uint32_t)(SRAMCAN_TBSA +                            \
                                  (STM32_FDCAN_TB_NBR * SRAMCAN_TB_SIZE)))

/* Message RAM size.*/
#define SRAMCAN_SIZE  ((uint32_t)(SRAMCAN_TMSA +                            \
                                  (STM32_FDCAN_TM_NBR * SRAMCAN_TM_SIZE)))

#define TIMEOUT_INIT_MS                 250U
#define TIMEOUT_CSA_MS                  250U

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief CAN1 driver identifier.*/
#if STM32_CAN_USE_FDCAN1 || defined(__DOXYGEN__)
CANDriver CAND1;
#endif

/** @brief CAN2 driver identifier.*/
#if STM32_CAN_USE_FDCAN2 || defined(__DOXYGEN__)
CANDriver CAND2;
#endif

/** @brief CAN3 driver identifier.*/
#if STM32_CAN_USE_FDCAN3 || defined(__DOXYGEN__)
CANDriver CAND3;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static const uint8_t dlc_to_bytes[] = {
  0U,  1U,  2U,  3U,  4U,  5U,  6U,  7U,
  8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static bool fdcan_clock_stop(CANDriver *canp) {
  systime_t start, end;

  /* Requesting clock stop then waiting for it to happen.*/
  canp->fdcan->CCCR |= FDCAN_CCCR_CSR;
  start = osalOsGetSystemTimeX();
  end = osalTimeAddX(start, TIME_MS2I(TIMEOUT_INIT_MS));
  while ((canp->fdcan->CCCR & FDCAN_CCCR_CSA) == 0U) {
    if (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), start, end)) {
      return true;
    }
    osalThreadSleepS(1);
  }

  return false;
}

static bool fdcan_init_mode(CANDriver *canp) {
  systime_t start, end;

  /* Going in initialization mode then waiting for it to happen.*/
  canp->fdcan->CCCR |= FDCAN_CCCR_INIT;
  start = osalOsGetSystemTimeX();
  end = osalTimeAddX(start, TIME_MS2I(TIMEOUT_INIT_MS));
  while ((canp->fdcan->CCCR & FDCAN_CCCR_INIT) == 0U) {
    if (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), start, end)) {
      return true;
    }
    osalThreadSleepS(1);
  }

  return false;
}

static bool fdcan_active_mode(CANDriver *canp) {
  systime_t start, end;

  /* Going in active mode then waiting for it to happen.*/
  canp->fdcan->CCCR &= ~FDCAN_CCCR_INIT;
  start = osalOsGetSystemTimeX();
  end = osalTimeAddX(start, TIME_MS2I(TIMEOUT_INIT_MS));
  while ((canp->fdcan->CCCR & FDCAN_CCCR_INIT) != 0U) {
    if (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), start, end)) {
      return true;
    }
    osalThreadSleepS(1);
  }

  return false;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level CAN driver initialization.
 *
 * @notapi
 */
void can_lld_init(void) {

  /* Unit reset.*/
  rccResetFDCAN();

#if STM32_CAN_USE_FDCAN1
  /* Driver initialization.*/
  canObjectInit(&CAND1);
  CAND1.fdcan = FDCAN1;
  CAND1.ram_base = ((uint32_t *)SRAMCAN_BASE + (CAN1_OFFSET_INSTANCE * SRAMCAN_SIZE));
#endif

#if STM32_CAN_USE_FDCAN2
  /* Driver initialization.*/
  canObjectInit(&CAND2);
  CAND2.fdcan = FDCAN2;
  CAND2.ram_base = ((uint32_t *)SRAMCAN_BASE + (CAN2_OFFSET_INSTANCE * SRAMCAN_SIZE));
#endif

#if STM32_CAN_USE_FDCAN3
  /* Driver initialization.*/
  canObjectInit(&CAND3);
  CAND3.fdcan = FDCAN3;
  CAND3.ram_base = (uint32_t *)(SRAMCAN_BASE + 2U * SRAMCAN_SIZE);
#endif

  /* Configure global CKDIV for STM32G4XX */
#if defined(STM32G4XX) && STM32_CAN_CKDIV != 0
  /* CAND1 needs to be put into configuration mode to allow CKDIV configuration */
  rccEnableFDCAN(true);
  if (fdcan_clock_stop(&CAND1)) {
    osalDbgAssert(false, "CAN clock stop failed, check clocks and pin config");
    return;
  }
  if (fdcan_init_mode(&CAND1)) {
    osalDbgAssert(false, "CAN initialization failed, check clocks and pin config");
    return;
  }
  CAND1.fdcan->CCCR |= FDCAN_CCCR_CCE;

  /* Write actual configuration into CKDIV register */
  FDCAN_CONFIG->CKDIV = STM32_CAN_CKDIV;

  rccDisableFDCAN();
#endif
}

/**
 * @brief   Configures and activates the CAN peripheral.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @return              The operation result.
 * @retval false        if the operation succeeded.
 * @retval true         if the operation failed.
 *
 * @notapi
 */
bool can_lld_start(CANDriver *canp) {

  /* Clock activation.*/
  rccEnableFDCAN(true);

  /* If it is the first activation then performing some extra
     initializations.*/
  for (uint32_t *wp = canp->ram_base;
       wp < canp->ram_base + SRAMCAN_SIZE;
       wp += 1U) {
    *wp = (uint32_t)0U;
  }

  /* Requesting clock stop.*/
  if (fdcan_clock_stop(canp)) {
    osalDbgAssert(false, "CAN clock stop failed, check clocks and pin config");
    return true;
  }

  /* Going in initialization mode.*/
  if (fdcan_init_mode(canp)) {
    osalDbgAssert(false, "CAN initialization failed, check clocks and pin config");
    return true;
  }

  /* Configuration can be performed now.*/
  canp->fdcan->CCCR |= FDCAN_CCCR_CCE;

  /* Setting up operation mode except driver-controlled bits.*/
  canp->fdcan->NBTP = canp->config->NBTP;
  canp->fdcan->DBTP = canp->config->DBTP;
  canp->fdcan->CCCR |= canp->config->CCCR;

  /* TEST is only writable when FDCAN_CCCR_TEST is set and FDCAN is still in
   * configuration mode */
  if (canp->config->CCCR & FDCAN_CCCR_TEST) {
	canp->fdcan->TEST = canp->config->TEST;
  }
#ifdef STM32G4XX
  canp->fdcan->RXGFC  = canp->config->RXGFC;
#elif defined(STM32H7XX)
  canp->fdcan->GFC    = canp->config->RXGFC;
#else
#error "Unsupported STM32 for FDCAN LLD driver"
#endif

#ifdef STM32H7XX

#if STM32_CAN_USE_FDCAN1
  if (&CAND1 == canp) {
    /* TODO: ADD MASK */
    /* H7 version of FDCAN has configurable memory layout, so configure it */
    canp->fdcan->SIDFC = (((STM32_FDCAN_FLS_NBR) << 16) | (((CAN1_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_FLSSA) << 2));
    canp->fdcan->XIDFC = (((STM32_FDCAN_FLE_NBR) << 16) | (((CAN1_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_FLESA) << 2));
    canp->fdcan->RXF0C = (((STM32_FDCAN_RF0_NBR) << 16) | (((CAN1_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_RF0SA) << 2));
    canp->fdcan->RXF1C = (((STM32_FDCAN_RF1_NBR) << 16) | (((CAN1_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_RF1SA) << 2));
    canp->fdcan->RXBC  = (((CAN1_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_RBSA) << 2);
    canp->fdcan->TXEFC = (((STM32_FDCAN_TEF_NBR) << 16) | (((CAN1_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_TEFSA) << 2));
    /* NB: this doesn't set NDTB, but sets TFQS to run in queue mode with no dedicated buffers */
    canp->fdcan->TXBC  = (((STM32_FDCAN_TB_NBR)  << 24) | (((CAN1_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_TBSA) << 2));
  }
#endif

#if STM32_CAN_USE_FDCAN2
  if (&CAND2 == canp) {
    /* TODO: ADD MASK */
    /* H7 version of FDCAN has configurable memory layout, so configure it */
    canp->fdcan->SIDFC = (((STM32_FDCAN_FLS_NBR) << 16) | (((CAN2_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_FLSSA) << 2));
    canp->fdcan->XIDFC = (((STM32_FDCAN_FLE_NBR) << 16) | (((CAN2_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_FLESA) << 2));
    canp->fdcan->RXF0C = (((STM32_FDCAN_RF0_NBR) << 16) | (((CAN2_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_RF0SA) << 2));
    canp->fdcan->RXF1C = (((STM32_FDCAN_RF1_NBR) << 16) | (((CAN2_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_RF1SA) << 2));
    canp->fdcan->RXBC  = (((CAN2_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_RBSA) << 2);
    canp->fdcan->TXEFC = (((STM32_FDCAN_TEF_NBR) << 16) | (((CAN2_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_TEFSA) << 2));
    /* NB: this doesn't set NDTB, but sets TFQS to run in queue mode with no dedicated buffers */
    canp->fdcan->TXBC  = (((STM32_FDCAN_TB_NBR)  << 24) | (((CAN2_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_TBSA) << 2));
  }
#endif

#if STM32_CAN_USE_FDCAN3
  if (&CAND3 == canp) {
    /* TODO: ADD MASK */
    /* H7 version of FDCAN has configurable memory layout, so configure it */
    canp->fdcan->SIDFC = (((STM32_FDCAN_FLS_NBR) << 16) | (((CAN3_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_FLSSA) << 2));
    canp->fdcan->XIDFC = (((STM32_FDCAN_FLE_NBR) << 16) | (((CAN3_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_FLESA) << 2));
    canp->fdcan->RXF0C = (((STM32_FDCAN_RF0_NBR) << 16) | (((CAN3_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_RF0SA) << 2));
    canp->fdcan->RXF1C = (((STM32_FDCAN_RF1_NBR) << 16) | (((CAN3_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_RF1SA) << 2));
    canp->fdcan->RXBC  = (((CAN3_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_RBSA) << 2);
    canp->fdcan->TXEFC = (((STM32_FDCAN_TEF_NBR) << 16) | (((CAN3_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_TEFSA) << 2));
    /* NB: this doesn't set NDTB, but sets TFQS to run in queue mode with no dedicated buffers */
    canp->fdcan->TXBC  = (((STM32_FDCAN_TB_NBR)  << 24) | (((CAN3_OFFSET_INSTANCE * SRAMCAN_SIZE) + SRAMCAN_TBSA) << 2));
  }
#endif

  /* TODO: ADD IN CONFIG DATA FIELD. */
  /* Data field. Data field sizes > 8
     bytes are intended for CAN FD operation only.  */
  canp->fdcan->TXESC = 0x0;
  canp->fdcan->RXESC = 0x0;

#endif /* STM32H7XX */

  /* Start clock and disable configuration mode.*/
  canp->fdcan->CCCR &= ~(FDCAN_CCCR_CSR);

  /* Enabling interrupts, only using interrupt zero.*/
  canp->fdcan->IR     = (uint32_t)-1;
  canp->fdcan->IE     = FDCAN_IE_RF1NE | FDCAN_IE_RF1LE |
                        FDCAN_IE_RF0NE | FDCAN_IE_RF0LE |
                        FDCAN_IE_TCE;
  canp->fdcan->TXBTIE = FDCAN_TXBTIE_TIE;
  canp->fdcan->ILE    = FDCAN_ILE_EINT0;

  /* Going in active mode.*/
  if (fdcan_active_mode(canp)) {
    osalDbgAssert(false, "CAN initialization failed, check clocks and pin config");
    return true;
  }

  return false;
}

/**
 * @brief   Deactivates the CAN peripheral.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_stop(CANDriver *canp) {

  /* If in ready state then disables the CAN peripheral.*/
  if (canp->state == CAN_READY) {
    /* Disabling and clearing interrupts.*/
    canp->fdcan->IE  = 0U;
    canp->fdcan->IR  = (uint32_t)-1;
    canp->fdcan->ILE = 0U;
    canp->fdcan->TXBTIE = 0U;

    /* Disables the peripheral.*/
    (void) fdcan_clock_stop(canp);

    rccDisableFDCAN();
  }
}

/**
 * @brief   Determines whether a frame can be transmitted.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 *
 * @return              The queue space availability.
 * @retval false        no space in the transmit queue.
 * @retval true         transmit slot available.
 *
 * @notapi
 */
bool can_lld_is_tx_empty(CANDriver *canp, canmbx_t mailbox) {

  (void)mailbox;

  return (bool)(((canp->fdcan->TXFQS & FDCAN_TXFQS_TFQF) == 0U) &&
                ((canp->fdcan->TXFQS & FDCAN_TXFQS_TFFL) > 0U));
}

/**
 * @brief   Inserts a frame into the transmit queue.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] ctfp      pointer to the CAN frame to be transmitted
 * @param[in] mailbox   mailbox number,  @p CAN_ANY_MAILBOX for any mailbox
 *
 * @notapi
 */
void can_lld_transmit(CANDriver *canp, canmbx_t mailbox, const CANTxFrame *ctfp) {
  uint32_t put_index = 0;
  uint32_t *tx_address = 0;

  (void)mailbox;

  osalDbgCheck(dlc_to_bytes[ctfp->DLC] <= CAN_MAX_DLC_BYTES);

  /* Retrieve the TX FIFO put index.*/
  put_index = ((canp->fdcan->TXFQS & FDCAN_TXFQS_TFQPI) >> FDCAN_TXFQS_TFQPI_Pos);

  /* Writing frame. */
  tx_address = canp->ram_base + (SRAMCAN_TBSA + (put_index * 4U)); //4U Element size in ram words

  *tx_address++ = ctfp->header32[0];
  *tx_address++ = ctfp->header32[1];
  for (unsigned i = 0U; i < dlc_to_bytes[ctfp->DLC]; i += 4U) {
    *tx_address++ = ctfp->data32[i / 4U];
  }

  /* Starting transmission.*/
  canp->fdcan->TXBAR = ((uint32_t)1 << put_index);
}

/**
 * @brief   Determines whether a frame has been received.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 *
 * @return              The queue space availability.
 * @retval false        no space in the transmit queue.
 * @retval true         transmit slot available.
 *
 * @notapi
 */
bool can_lld_is_rx_nonempty(CANDriver *canp, canmbx_t mailbox) {

  switch (mailbox) {
    case CAN_ANY_MAILBOX:
      return can_lld_is_rx_nonempty(canp, 1U) ||
             can_lld_is_rx_nonempty(canp, 2U);
    case 1:
      return (bool)((canp->fdcan->RXF0S & FDCAN_RXF0S_F0FL) != 0U);
    case 2:
      return (bool)((canp->fdcan->RXF1S & FDCAN_RXF1S_F1FL) != 0U);
    default:
      return false;
  }
}

/**
 * @brief   Receives a frame from the input queue.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 * @param[out] crfp     pointer to the buffer where the CAN frame is copied
 *
 * @notapi
 */
void can_lld_receive(CANDriver *canp, canmbx_t mailbox, CANRxFrame *crfp) {
  uint32_t get_index;
  uint32_t *rx_address;

  if (mailbox == CAN_ANY_MAILBOX) {
    if (can_lld_is_rx_nonempty(canp, 1U)) {
      mailbox = 1U;
    }
    else if (can_lld_is_rx_nonempty(canp, 2U)) {
      mailbox = 2U;
    }
    else {
      return;
    }
  }

  if (mailbox == 1U) {
     /* GET index RXF0, add it and the length to the rx_address.*/
     get_index = (canp->fdcan->RXF0S & FDCAN_RXF0S_F0GI_Msk) >> FDCAN_RXF0S_F0GI_Pos;
     rx_address = canp->ram_base + ((SRAMCAN_RF0SA + (get_index * 4U))); //4U Element size in ram words
  }
  else {
     /* GET index RXF1, add it and the length to the rx_address.*/
     get_index = (canp->fdcan->RXF1S & FDCAN_RXF1S_F1GI_Msk) >> FDCAN_RXF1S_F1GI_Pos;
     rx_address = canp->ram_base + ((SRAMCAN_RF1SA + (get_index * 4U))); //4U Element size in ram words
  }
  crfp->header32[0] = *rx_address++;
  crfp->header32[1] = *rx_address++;

  /* Copy message from FDCAN peripheral's SRAM to structure. RAM is restricted
     to word aligned accesses, so up to 3 extra bytes may be copied.*/
  for (unsigned i = 0U; i < dlc_to_bytes[crfp->DLC]; i += 4U) {
    crfp->data32[i / 4U] = *rx_address++;
  }

  /* Acknowledge receipt by writing the get-index to the acknowledge
     register RXFxA then re-enable RX FIFO message arrived interrupt once
     the FIFO is emptied.*/
  if (mailbox == 1U) {
    uint32_t rxf0a = canp->fdcan->RXF0A;
    rxf0a &= ~FDCAN_RXF0A_F0AI_Msk;
    rxf0a |= get_index << FDCAN_RXF0A_F0AI_Pos;
    canp->fdcan->RXF0A = rxf0a;

    if (!can_lld_is_rx_nonempty(canp, mailbox)) {
      canp->fdcan->IE |= FDCAN_IE_RF0NE;
    }
  }
  else {
    uint32_t rxf1a = canp->fdcan->RXF1A;
    rxf1a &= ~FDCAN_RXF1A_F1AI_Msk;
    rxf1a |= get_index << FDCAN_RXF1A_F1AI_Pos;
    canp->fdcan->RXF1A = rxf1a;

    if (!can_lld_is_rx_nonempty(canp, mailbox)) {
      canp->fdcan->IE |= FDCAN_IE_RF1NE;
    }
  }
}

/**
 * @brief   Tries to abort an ongoing transmission.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number
 *
 * @notapi
 */
void can_lld_abort(CANDriver *canp, canmbx_t mailbox) {

  (void)canp;
  (void)mailbox;
}

#if CAN_USE_SLEEP_MODE || defined(__DOXYGEN__)
/**
 * @brief   Enters the sleep mode.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_sleep(CANDriver *canp) {

  (void)canp;
}

/**
 * @brief   Enforces leaving the sleep mode.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_wakeup(CANDriver *canp) {

  (void)canp;
}
#endif /* CAN_USE_SLEEP_MODE */

/**
 * @brief   FDCAN IRQ0 service routine.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_serve_interrupt(CANDriver *canp) {
  uint32_t ir = 0;

  /* Getting and clearing active IRQs.*/
  ir = canp->fdcan->IR;
  canp->fdcan->IR = ir;

  /* RX events.*/
  if ((ir & FDCAN_IR_RF0N) != 0U) {
    /* Disabling this source until the queue is emptied.*/
    canp->fdcan->IE &= ~FDCAN_IE_RF0NE;
    _can_rx_full_isr(canp, CAN_MAILBOX_TO_MASK(1U));
  }
  if ((ir & FDCAN_IR_RF1N) != 0U) {
    /* Disabling this source until the queue is emptied.*/
    canp->fdcan->IE &= ~FDCAN_IE_RF1NE;
    _can_rx_full_isr(canp, CAN_MAILBOX_TO_MASK(2U));
  }

  /* Overflow events.*/
  if (((ir & FDCAN_IR_RF0L) != 0U) || ((ir & FDCAN_IR_RF1L) != 0U)) {
    _can_error_isr(canp, CAN_OVERFLOW_ERROR);
  }

  /* TX events.*/
  if ((ir & FDCAN_IR_TC) != 0U) {
    eventflags_t flags = 0U;

    flags |= 1U;
    _can_tx_empty_isr(canp, flags);
  }
}

#endif /* HAL_USE_CAN */

/** @} */
