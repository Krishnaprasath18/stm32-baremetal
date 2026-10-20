
#include <stdint.h>

/* ---------- Base addresses ---------- */
#define GPIOA_BASE          0x40010800
#define GPIOC_BASE          0x40011000
#define RCC_BASE            0x40021000
#define AFIO_BASE           0x40010000
#define EXTI_BASE           0x40010400
#define TIM2_BASE           0x40000000
#define NVIC_ISER0_ADDRESS  0xE000E100   /* IRQn 0-31 live here */
#define NVIC_ISER1_ADDRESS  0xE000E104   /* IRQn 32-63 live here */

/* ---------- GPIO offsets ---------- */
#define GPIOx_CRL_OFFSET    0x00
#define GPIOx_CRH_OFFSET    0x04
#define GPIOx_IDR_OFFSET    0x08
#define GPIOx_ODR_OFFSET    0x0C

/* ---------- AFIO / EXTI offsets ---------- */
#define AFIO_EXTI4_OFFSET   0x14
#define EXTI_IMR_OFFSET     0x00
#define EXTI_RTSR_OFFSET    0x08
#define EXTI_FTSR_OFFSET    0x0C
#define EXTI_PR_OFFSET      0x14

/* ---------- RCC offsets ---------- */
#define RCC_APB2ENR_OFFSET  0x18
#define RCC_APB1ENR_OFFSET  0x1C   /* NEW — TIM2 lives on APB1, not APB2 */

/* ---------- TIM2 offsets ---------- */
#define TIM2_CR1_OFFSET     0x00   /* NEW — bit0 = CEN, counter enable */
#define TIM2_DIER_OFFSET    0x0C   /* NEW — bit0 = UIE, update interrupt enable */
#define TIM2_SR_OFFSET      0x10   /* NEW — bit0 = UIF, update interrupt flag */
#define TIM2_PSC_OFFSET     0x28
#define TIM2_ARR_OFFSET     0x2C

/* ---------- Full addresses ---------- */
#define GPIOA_CRL_ADDRESS       (GPIOA_BASE + GPIOx_CRL_OFFSET)
#define GPIOA_ODR_ADDRESS       (GPIOA_BASE + GPIOx_ODR_OFFSET)
#define GPIOC_IDR_ADDRESS       (GPIOC_BASE + GPIOx_IDR_OFFSET)
#define GPIOC_CRH_ADDRESS       (GPIOC_BASE + GPIOx_CRH_OFFSET)
#define RCC_APB2ENR_ADDRESS     (RCC_BASE  + RCC_APB2ENR_OFFSET)
#define RCC_APB1ENR_ADDRESS     (RCC_BASE  + RCC_APB1ENR_OFFSET)
#define AFIO_EXTI4_ADDRESS      (AFIO_BASE + AFIO_EXTI4_OFFSET)
#define EXTI_IMR_ADDRESS        (EXTI_BASE + EXTI_IMR_OFFSET)
#define EXTI_FTSR_ADDRESS       (EXTI_BASE + EXTI_FTSR_OFFSET)
#define EXTI_RTSR_ADDRESS       (EXTI_BASE + EXTI_RTSR_OFFSET)
#define EXTI_PR_ADDRESS         (EXTI_BASE + EXTI_PR_OFFSET)
#define TIM2_CR1_ADDRESS        (TIM2_BASE + TIM2_CR1_OFFSET)
#define TIM2_DIER_ADDRESS       (TIM2_BASE + TIM2_DIER_OFFSET)
#define TIM2_SR_ADDRESS         (TIM2_BASE + TIM2_SR_OFFSET)
#define TIM2_PSC_ADDRESS        (TIM2_BASE + TIM2_PSC_OFFSET)
#define TIM2_ARR_ADDRESS        (TIM2_BASE + TIM2_ARR_OFFSET)

/* ---------- File-scope pointers (shared between main() and both ISRs) ---------- */
volatile uint32_t *pGpioCrlAdd     = (volatile uint32_t*) GPIOA_CRL_ADDRESS;
volatile uint32_t *pGpioOdrAdd     = (volatile uint32_t*) GPIOA_ODR_ADDRESS;
volatile uint32_t *pRccApb2EnbAdd  = (volatile uint32_t*) RCC_APB2ENR_ADDRESS;
volatile uint32_t *pRccApb1EnbAdd  = (volatile uint32_t*) RCC_APB1ENR_ADDRESS;
volatile uint32_t *pGpiocIdrAdd    = (volatile uint32_t*) GPIOC_IDR_ADDRESS;
volatile uint32_t *pGpiocCrhAdd    = (volatile uint32_t*) GPIOC_CRH_ADDRESS;
volatile uint32_t *pAfioExti4Add   = (volatile uint32_t*) AFIO_EXTI4_ADDRESS;
volatile uint32_t *pExtiImrAdd     = (volatile uint32_t*) EXTI_IMR_ADDRESS;
volatile uint32_t *pExtiFtsrAdd    = (volatile uint32_t*) EXTI_FTSR_ADDRESS;
volatile uint32_t *pExtiRtsrAdd    = (volatile uint32_t*) EXTI_RTSR_ADDRESS;
volatile uint32_t *pExtiPrAdd      = (volatile uint32_t*) EXTI_PR_ADDRESS;
volatile uint32_t *pNvicIser1      = (volatile uint32_t*) NVIC_ISER1_ADDRESS;   /* for EXTI15_10, IRQn 40 */
volatile uint32_t *pNvicIser0      = (volatile uint32_t*) NVIC_ISER0_ADDRESS;   /* for TIM2, IRQn 28 */
volatile uint32_t *pTim2Cr1Add     = (volatile uint32_t*) TIM2_CR1_ADDRESS;
volatile uint32_t *pTim2DierAdd    = (volatile uint32_t*) TIM2_DIER_ADDRESS;
volatile uint32_t *pTim2SrAdd      = (volatile uint32_t*) TIM2_SR_ADDRESS;
volatile uint32_t *pTim2PscAdd     = (volatile uint32_t*) TIM2_PSC_ADDRESS;
volatile uint32_t *pTim2ArrAdd     = (volatile uint32_t*) TIM2_ARR_ADDRESS;

/* ---------- Shared state between the two ISRs ---------- */
volatile uint8_t  blinking_active = 0;
volatile uint32_t tick_count = 0;

/* ---------- EXTI ISR: button press detected (falling or rising edge) ---------- */
void EXTI15_10_IRQHandler(void)
{
    if ( (*pGpiocIdrAdd & (1<<13)) == 0 )   /* pressed */
    {
        blinking_active = 1;   /* start/restart the 5-second blink sequence */
        tick_count = 0;
    }
    /* if released (else case), we don't need to do anything special here —
       the blink sequence runs on its own timeline once started */

    *pExtiPrAdd |= (1<<13);   /* EXTI_PR: write-1-to-clear, as before */
}

/* ---------- TIM2 ISR: fires every 250ms (ARR=249, so every 250ms a FULL period) ---------- */
void TIM2_IRQHandler(void)
{
    *pTim2SrAdd &= ~(0x1<<0);   /* TIM2_SR: write-0-to-clear — OPPOSITE of EXTI_PR, do not use |= here */

    if (blinking_active)
    {
        *pGpioOdrAdd ^= (0x1<<5);   /* toggle LED */
        tick_count++;

        if (tick_count >= 20)   /* 20 x 250ms = 5000ms = 5 seconds */
        {
            blinking_active = 0;
            *pGpioOdrAdd |= (0x1<<5);   /* force LED back to steady ON, not whatever toggle state it landed on */
        }
    }
}

int main(void)
{
    /* Enable clocks: GPIOA, GPIOC, AFIO (all APB2) */
    *pRccApb2EnbAdd |= ( (1<<2) | (1<<4) | (1<<0) );

    /* Enable TIM2 clock (APB1, bit 0) — NEW */
    *pRccApb1EnbAdd |= (1<<0);

    /* PA5 as push-pull output */
    *pGpioCrlAdd &= ~(0xF<<20);
    *pGpioCrlAdd |=  (0x2<<20);

    /* PA5 starts HIGH — "normally ON" per your spec */
    *pGpioOdrAdd |= (0x1<<5);

    /* PC13 as floating input */
    *pGpiocCrhAdd &= ~(0xF<<20);
    *pGpiocCrhAdd |=  (0x4<<20);

    /* AFIO: route EXTI line 13 to GPIOC */
    *pAfioExti4Add |= (0x02<<4);

    /* EXTI: both edges, unmask line 13 */
    *pExtiFtsrAdd |= (1<<13);
    *pExtiRtsrAdd |= (1<<13);
    *pExtiImrAdd  |= (1<<13);

    /* NVIC: enable EXTI15_10 (IRQn 40 -> ISER1, bit 8) */
    *pNvicIser1 |= (1<<8);

    /* TIM2 setup: 8MHz / 8000 = 1kHz -> 1ms/tick, ARR=249 -> 250ms period */
    *pTim2PscAdd = 7999;
    *pTim2ArrAdd = 249;
    *pTim2DierAdd |= (1<<0);   /* enable update interrupt */

    /* NVIC: enable TIM2 (IRQn 28 -> ISER0, bit 28) — work out this math yourself tonight */
    *pNvicIser0 |= (1<<28);

    *pTim2Cr1Add |= (1<<0);   /* CEN — start the counter. Without this, nothing above runs. */

    while (1)
    {
    }
}

