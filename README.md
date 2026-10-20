# STM32F103 Bare-Metal: Button Interrupt + Timer-Driven LED Blink

Bare-metal Embedded C firmware for the **STM32F103RB (Nucleo-F103RB)**, written at **register level with no HAL library**.

The LED stays ON. When the user button is pressed, the LED blinks for 5 seconds and then returns to steady ON. All the work happens in interrupt handlers, so the main loop is empty.

## Hardware

| Pin | Function | Configuration |
|---|---|---|
| PA5 | On-board user LED (LD2) | Output, push-pull, 2 MHz |
| PC13 | On-board user button (B1), active LOW | Floating input (external pull-up on board) |

Clock: default 8 MHz HSI, no PLL.

## How it works

```
Button (PC13) -> AFIO EXTICR4 -> EXTI line 13 -> NVIC (IRQ 40) -> EXTI15_10_IRQHandler()
                                                                    sets blinking_active = 1

TIM2 update (every 250 ms) -> NVIC (IRQ 28) -> TIM2_IRQHandler()
                                                toggles LED while blinking_active,
                                                stops after 20 toggles (5 s)
```

1. **RCC**: enable the clocks for GPIOA, GPIOC and AFIO (APB2ENR) and TIM2 (APB1ENR).
2. **GPIO**: PA5 as an output (CRL bits 20–23), PC13 as an input (CRH bits 20–23).
3. **AFIO**: route EXTI line 13 to port C (EXTICR4, bits 4–7 = `0b0010`).
4. **EXTI**: trigger on both edges (RTSR/FTSR) and unmask line 13 (IMR).
5. **NVIC**: enable IRQ 40 (ISER1 bit 8) for EXTI15_10 and IRQ 28 (ISER0 bit 28) for TIM2.
6. **TIM2**: prescaler and auto-reload set for a 250 ms update interrupt, then start the counter (CEN).

## Timer calculation

```
Timer clock     = 8 MHz
PSC = 7999      -> 8,000,000 / (7999 + 1) = 1000 Hz  -> 1 count = 1 ms
ARR = 249       -> (249 + 1) counts       = 250 ms update period
Blink duration  = 20 updates x 250 ms     = 5 s
```

## Key points

- **`volatile`** on the register pointers and on variables shared between ISRs, so the compiler always reads them fresh.
- **Interrupt flags are cleared differently:** EXTI_PR is *write-1-to-clear*, while TIM2_SR UIF is cleared by *writing 0*.
- Each register is changed with read-modify-write (`|=`, `&= ~`) so the other bits stay as they were.

## Build and flash

- IDE: STM32CubeIDE
- Programmer: ST-Link (on-board on the Nucleo)
- Open the project, build it, then Run/Debug.

## Reference

- RM0008: STM32F101xx/F102xx/F103xx Reference Manual (RCC, GPIO, AFIO, EXTI, TIM2)
- PM0056: Cortex-M3 Programming Manual (NVIC)

## Author

Krishna Prasath P · [GitHub](https://github.com/Krishnaprasath18)
