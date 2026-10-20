# UART: Complete Notes (Basic to Advanced)

Board used in examples: **Nucleo-F103RB (STM32F103RB)**, 8 MHz HSI clock, USART2 on PA2/PA3.
Reference: RM0008, Chapter 27 (USART).

---

## Part 1: Basics

### 1.1 What is UART?

**UART** = Universal Asynchronous Receiver/Transmitter.

It is the simplest way for two chips (or a chip and a PC) to exchange bytes. It sends data **one bit at a time** over a single wire, so it is called **serial** communication.

- **Universal**: baud rate, data size, parity and stop bits can all be configured.
- **Asynchronous**: there is **no clock wire**. Both sides agree on a speed in advance (the baud rate), and each side uses its own clock to time the bits.
- **Receiver/Transmitter**: it can send and receive at the same time (**full duplex**).

### 1.2 The wires

Only 3 wires are needed:

| Wire | Purpose |
|---|---|
| **TX** | Transmit: data goes OUT of this pin |
| **RX** | Receive: data comes IN to this pin |
| **GND** | Common ground: both sides must share it |

**Important:** TX of one device connects to RX of the other (they cross).

```
   STM32                      PC / other MCU
   TX  ──────────────────────►  RX
   RX  ◄──────────────────────  TX
   GND ────────────────────────  GND
```

The #1 wiring bug: connecting TX to TX. Nothing happens, no error, just silence.

### 1.3 UART vs USART

- **UART**: asynchronous only (no clock wire).
- **USART**: Universal **Synchronous**/Asynchronous. It can also work with a clock wire (CK pin) in synchronous mode. The STM32F103 has **USART** peripherals, but 99% of the time we use them in plain UART (asynchronous) mode.

### 1.4 Where UART is used

- Printing debug messages from the MCU to the PC (the most common use, you will do this daily).
- GPS modules, Bluetooth modules (HC-05), GSM modules, Wi‑Fi modules (ESP8266 AT commands).
- Bootloaders (flashing firmware over serial).
- Between two microcontrollers.
- RS‑232 and RS‑485 industrial links (UART with different voltage levels).

---

## Part 2: The UART frame (how one byte travels)

### 2.1 Idle state

When nothing is sent, the TX line stays **HIGH** (logic 1). This is the idle state.

### 2.2 Frame format

One byte is sent as a **frame**:

```
 idle  START   D0  D1  D2  D3  D4  D5  D6  D7  (PARITY)  STOP  idle
 ────┐      ┌────┐    ┌────────┐        ┌────┐          ┌──────────
     │      │    │    │        │        │    │          │
     └──────┘    └────┘        └────────┘    └──────────┘
      LOW    ... data bits, LSB first ...            HIGH
```

| Part | Bits | Level | Meaning |
|---|---|---|---|
| **Start bit** | 1 | LOW | "A byte is coming." The receiver sees the falling edge and starts its timer. |
| **Data bits** | 5–9 (usually 8) | 0/1 | The actual data, sent **LSB (bit 0) first** |
| **Parity bit** | 0 or 1 | 0/1 | Optional error check |
| **Stop bit(s)** | 1, 1.5 or 2 | HIGH | "Byte finished." Gives the receiver time to get ready. |

The most common setting is written **8N1**: 8 data bits, No parity, 1 stop bit. One frame is 1 + 8 + 1 = **10 bits**.

### 2.3 Example: sending the letter 'A'

'A' = 0x41 = binary 0100 0001.
LSB first, so the wire shows: **1, 0, 0, 0, 0, 0, 1, 0**.

```
Line:  1  | 0  | 1  0  0  0  0  0  1  0 | 1
       idle start  D0 D1 D2 D3 D4 D5 D6 D7  stop
```

### 2.4 Parity (error checking)

Parity adds one extra bit so the total number of 1s is even (**even parity**) or odd (**odd parity**).

- Data 0x41 has two 1s. With even parity, the parity bit = 0 (total stays even). With odd parity, parity bit = 1.
- The receiver counts the 1s. If the count is wrong, it sets a **Parity Error** flag.
- It can detect a single flipped bit, but not two flipped bits. Most debug links use no parity.

### 2.5 How the receiver reads bits without a clock

The receiver knows the baud rate, so it knows how long each bit lasts (the **bit time**).

1. It waits for the falling edge of the start bit.
2. It waits **half a bit time** to reach the middle of the start bit.
3. It then samples every **one bit time** after that, so it always reads the **middle** of each bit, where the signal is stable.

The STM32 actually samples each bit 16 times (**oversampling by 16**) and takes a majority vote of the 3 middle samples. That is why the number 16 appears in the baud rate formula.

---

## Part 3: Baud rate

### 3.1 What is baud rate?

Baud rate = number of **bits per second** on the wire (for UART, 1 baud = 1 bit/s).

Common values: 9600, 19200, 38400, 57600, **115200**, 230400, 921600.

### 3.2 Bit time

```
bit time = 1 / baud rate

9600 baud    → 1 / 9600    = 104.2 µs per bit
115200 baud  → 1 / 115200  = 8.68 µs per bit
```

### 3.3 How fast is the real data?

At 8N1, each byte takes 10 bits.

```
115200 baud → 11,520 bytes per second → about 11.5 KB/s
9600 baud   →    960 bytes per second
```

So printing a 50‑character line at 9600 takes 50 / 960 ≈ **52 ms**. Keep this in mind when printing inside fast loops.

### 3.4 Both sides must match

Baud rate, data bits, parity and stop bits must be **identical** on both sides. If the baud rate is wrong you get **garbage characters** on the terminal (like `ÿ�Ê`), not silence. Garbage = wrong baud. Silence = wrong wiring or TX not enabled.

### 3.5 Allowed error

Clocks are never perfect. UART tolerates roughly **±2–3%** total baud rate error. More than that and bits are sampled in the wrong place, giving framing errors.

---

## Part 4: Voltage levels and hardware

### 4.1 TTL / CMOS UART (what the STM32 uses)

- Logic 1 = 3.3 V (STM32) or 5 V (Arduino, some modules).
- Logic 0 = 0 V.
- Only for short distances (less than ~1 m).

The STM32F103 pins are **3.3 V**. Many pins are 5 V‑tolerant as **inputs** (check the datasheet; PA2/PA3 are not 5 V tolerant), but the TX pin can never output 5 V. When connecting to a 5 V device, use a level shifter or a resistor divider on the STM32 RX pin.

### 4.2 RS‑232 (PC serial port, older industrial)

- Same UART frame, but **different voltages**: logic 1 = −3 to −15 V, logic 0 = +3 to +15 V (inverted and high voltage).
- Needs a converter chip (**MAX232**). Never connect RS‑232 directly to a microcontroller pin: it will damage it.
- Range: up to about 15 m.

### 4.3 RS‑485 (industrial, long distance)

- Same UART frame, but sent as a **differential pair** (A and B wires), like CAN.
- Needs a transceiver (**MAX485**). Half duplex: one wire pair, one talker at a time, with a Direction (DE/RE) pin.
- Range: up to 1200 m, many devices on one bus (Modbus RTU uses this).

### 4.4 USB‑to‑serial

A PC has no UART pins. A **USB‑to‑serial converter** (CP2102, CH340, FT232) creates a **virtual COM port** on the PC.

On the **Nucleo board you already have one**: the ST‑Link chip includes a Virtual COM Port (VCP). It is wired to **USART2 on PA2 (TX) and PA3 (RX)**. So you plug in the normal USB cable, and the PC sees a COM port. No extra hardware needed.

### 4.5 Terminal software on the PC

- **Tera Term** (recommended, simple), **PuTTY**, or the built‑in serial console in STM32CubeIDE.
- Find the port in Windows **Device Manager → Ports (COM & LPT) → "STMicroelectronics STLink Virtual COM Port (COMx)"**.
- Set: baud 9600 (or whatever you chose), 8 data bits, no parity, 1 stop bit, no flow control.

---

## Part 5: USART on the STM32F103

### 5.1 The USART peripherals

| Peripheral | Bus | Clock enable | TX pin | RX pin | IRQ number |
|---|---|---|---|---|---|
| USART1 | APB2 | APB2ENR bit 14 | PA9 | PA10 | 37 |
| **USART2** | **APB1** | **APB1ENR bit 17** | **PA2** | **PA3** | **38** |
| USART3 | APB1 | APB1ENR bit 18 | PB10 | PB11 | 39 |

We use **USART2** because it goes to the ST‑Link VCP on the Nucleo.

### 5.2 Base address and registers

USART2 base address: **0x40004400**

| Register | Offset | Purpose |
|---|---|---|
| **SR** | 0x00 | Status: flags that tell you what is happening |
| **DR** | 0x04 | Data: write a byte to send, read a byte received |
| **BRR** | 0x08 | Baud rate |
| **CR1** | 0x0C | Control 1: enable, data bits, parity, interrupts |
| **CR2** | 0x10 | Control 2: stop bits, clock settings |
| **CR3** | 0x14 | Control 3: flow control, DMA |
| GTPR | 0x18 | Guard time / prescaler (IrDA, smartcard; rarely used) |

### 5.3 SR: Status Register (read this to know what to do)

| Bit | Name | Meaning |
|---|---|---|
| 0 | **PE** | Parity Error |
| 1 | **FE** | Framing Error (stop bit was not HIGH; usually wrong baud rate) |
| 2 | **NE** | Noise Error |
| 3 | **ORE** | Overrun Error (a new byte arrived before you read the old one; the old one is lost) |
| 4 | **IDLE** | Idle line detected |
| 5 | **RXNE** | **Receive data register Not Empty: a byte has arrived, read DR now** |
| 6 | **TC** | **Transmission Complete: the last byte has fully left the wire** |
| 7 | **TXE** | **Transmit data register Empty: you can write the next byte to DR** |
| 8 | LBD | LIN break detected |
| 9 | CTS | CTS flag (flow control) |

The three you will use every day: **TXE, TC, RXNE**.

**TXE vs TC:** The USART has a transmit register (DR) and a shift register. When you write DR, the byte is copied to the shift register and TXE becomes 1 again (DR is free). TC becomes 1 only when the shift register has pushed the last bit out onto the wire. Use TXE to send bytes back‑to‑back; use TC before switching off the USART or disabling an RS‑485 driver.

### 5.4 DR: Data Register

- Writing to DR sends a byte (only the low 8 or 9 bits matter).
- Reading DR gives the received byte **and clears RXNE automatically**.
- It is really two separate registers (TDR and RDR) at the same address.

### 5.5 BRR: Baud Rate Register

```
Bits 15:4 = DIV_Mantissa (integer part)
Bits  3:0 = DIV_Fraction (fractional part, in 1/16 steps)
```

Formula:

```
USARTDIV = fPCLK / (16 × baud)
```

- fPCLK for USART2 is **PCLK1 (APB1 clock)**. With the default 8 MHz HSI and no prescaler, PCLK1 = **8 MHz**.
- fPCLK for USART1 is PCLK2 (APB2 clock).

**Example 1: 9600 baud at 8 MHz**

```
USARTDIV = 8,000,000 / (16 × 9600) = 52.083
Mantissa = 52             = 0x34
Fraction = 0.083 × 16     = 1.33 → round to 1 = 0x1
BRR = (52 << 4) | 1       = 0x341
```

**Example 2: 115200 baud at 8 MHz**

```
USARTDIV = 8,000,000 / (16 × 115200) = 4.34
Mantissa = 4              = 0x4
Fraction = 0.34 × 16      = 5.44 → round to 5 = 0x5
BRR = (4 << 4) | 5        = 0x45
```

Actual baud = 8,000,000 / (16 × 4.3125) = 115,942 → error 0.64%. Fine (under 2%).

**Shortcut:** `BRR = fPCLK / baud` (integer division) gives the same answer because (mantissa × 16 + fraction) = USARTDIV × 16 = fPCLK / baud. For 9600: 8,000,000 / 9600 = 833 = 0x341. Same result. Use the long formula in interviews; use the shortcut in code.

**Example 3: 115200 baud at 72 MHz (after PLL, APB1 = 36 MHz)**

```
USARTDIV = 36,000,000 / (16 × 115200) = 19.53
Mantissa = 19 = 0x13, Fraction = 0.53 × 16 = 8.5 → 8 (or 9)
BRR = 0x138
```

### 5.6 CR1: Control Register 1

| Bit | Name | Meaning |
|---|---|---|
| 0 | SBK | Send break |
| 1 | RWU | Receiver wakeup (mute mode) |
| 2 | **RE** | **Receiver Enable** |
| 3 | **TE** | **Transmitter Enable** |
| 4 | IDLEIE | Interrupt when the line goes idle |
| 5 | **RXNEIE** | **Interrupt when a byte is received** |
| 6 | TCIE | Interrupt when transmission complete |
| 7 | TXEIE | Interrupt when TX register empty |
| 8 | PEIE | Parity error interrupt |
| 9 | PS | Parity selection: 0 = even, 1 = odd |
| 10 | PCE | Parity control enable |
| 11 | WAKE | Wakeup method |
| 12 | **M** | Word length: 0 = 8 data bits, 1 = 9 data bits |
| 13 | **UE** | **USART Enable (master switch)** |

Minimum to work: **UE | TE | RE** = (1<<13) | (1<<3) | (1<<2) = **0x200C**.

### 5.7 CR2: Control Register 2

| Bits | Name | Meaning |
|---|---|---|
| 13:12 | **STOP** | 00 = 1 stop bit, 01 = 0.5, 10 = 2 stop bits, 11 = 1.5 |
| 11 | CLKEN | Clock pin enable (synchronous mode) |
| 10 | CPOL | Clock polarity (sync mode) |
| 9 | CPHA | Clock phase (sync mode) |
| 14 | LINEN | LIN mode enable |

Leave CR2 = 0 for 1 stop bit.

### 5.8 CR3: Control Register 3

| Bit | Name | Meaning |
|---|---|---|
| 6 | **DMAR** | DMA enable for receiver |
| 7 | **DMAT** | DMA enable for transmitter |
| 8 | **RTSE** | RTS flow control enable |
| 9 | **CTSE** | CTS flow control enable |
| 3 | HDSEL | Half‑duplex (single wire) mode |

### 5.9 GPIO configuration for the UART pins

On the F103 the pins must be put into **Alternate Function** mode:

| Pin | Role | CNF:MODE (4 bits) | Value | Meaning |
|---|---|---|---|---|
| PA2 | TX | 10:11 | **0xB** | Alternate function push‑pull, 50 MHz |
| PA3 | RX | 01:00 | **0x4** | Floating input (or 0x8 with pull‑up) |

PA2 is pin 2 → CRL bits **8–11**. PA3 is pin 3 → CRL bits **12–15**.

Also enable the **AFIO clock** (APB2ENR bit 0) whenever you use alternate functions.

---

## Part 6: Register‑level code (polling)

### 6.1 Definitions

```c
#include <stdint.h>

#define RCC_BASE        0x40021000
#define RCC_APB2ENR     (*(volatile uint32_t*)(RCC_BASE + 0x18))
#define RCC_APB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x1C))

#define GPIOA_BASE      0x40010800
#define GPIOA_CRL       (*(volatile uint32_t*)(GPIOA_BASE + 0x00))

#define USART2_BASE     0x40004400
#define USART2_SR       (*(volatile uint32_t*)(USART2_BASE + 0x00))
#define USART2_DR       (*(volatile uint32_t*)(USART2_BASE + 0x04))
#define USART2_BRR      (*(volatile uint32_t*)(USART2_BASE + 0x08))
#define USART2_CR1      (*(volatile uint32_t*)(USART2_BASE + 0x0C))

/* SR bits */
#define USART_SR_RXNE   (1 << 5)
#define USART_SR_TC     (1 << 6)
#define USART_SR_TXE    (1 << 7)

/* CR1 bits */
#define USART_CR1_RE    (1 << 2)
#define USART_CR1_TE    (1 << 3)
#define USART_CR1_RXNEIE (1 << 5)
#define USART_CR1_UE    (1 << 13)
```

Note the new style: `(*(volatile uint32_t*)ADDRESS)` puts the `*` inside the macro, so you write `USART2_DR = 'A';` instead of `*pUsart2Dr = 'A';`. This is how the official STM32 header files do it.

### 6.2 Initialisation

```c
void uart2_init(uint32_t baud)
{
    /* 1. Clocks: GPIOA + AFIO on APB2, USART2 on APB1 */
    RCC_APB2ENR |= (1 << 2) | (1 << 0);
    RCC_APB1ENR |= (1 << 17);

    /* 2. PA2 = TX: alternate function push-pull, 50 MHz (0xB) */
    GPIOA_CRL &= ~(0xF << 8);
    GPIOA_CRL |=  (0xB << 8);

    /* 3. PA3 = RX: floating input (0x4) */
    GPIOA_CRL &= ~(0xF << 12);
    GPIOA_CRL |=  (0x4 << 12);

    /* 4. Baud rate: PCLK1 = 8 MHz */
    USART2_BRR = 8000000 / baud;      /* 9600 -> 833 = 0x341 */

    /* 5. Enable transmitter, receiver and the USART itself */
    USART2_CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}
```

Order matters: clocks first, then GPIO, then baud, then enable.

### 6.3 Sending

```c
void uart2_write_char(char c)
{
    while (!(USART2_SR & USART_SR_TXE));   /* wait until DR is free */
    USART2_DR = c;
}

void uart2_write_string(const char *s)
{
    while (*s)
    {
        uart2_write_char(*s++);
    }
    while (!(USART2_SR & USART_SR_TC));    /* optional: wait until the last bit is out */
}
```

### 6.4 Receiving

```c
char uart2_read_char(void)
{
    while (!(USART2_SR & USART_SR_RXNE));  /* wait until a byte arrives */
    return (char)USART2_DR;                /* reading DR clears RXNE */
}
```

### 6.5 Echo program

```c
int main(void)
{
    uart2_init(9600);
    uart2_write_string("UART ready\r\n");

    while (1)
    {
        char c = uart2_read_char();
        uart2_write_char(c);               /* send it back */
    }
}
```

Type on the PC terminal, and each character comes back. `\r\n` = carriage return + line feed; terminals usually need both to start a new line.

### 6.6 Sending numbers

UART sends bytes, not numbers. To show the number 123 you must convert it to the characters '1', '2', '3'.

```c
void uart2_write_uint(uint32_t n)
{
    char buf[11];
    int i = 0;

    if (n == 0) { uart2_write_char('0'); return; }

    while (n > 0)
    {
        buf[i++] = '0' + (n % 10);         /* last digit first */
        n /= 10;
    }
    while (i > 0)
    {
        uart2_write_char(buf[--i]);        /* print in reverse */
    }
}
```

This is a classic interview exercise: "print an integer without printf".

### 6.7 Command parser (LED on/off)

```c
char cmd[16];
uint8_t idx = 0;

while (1)
{
    char c = uart2_read_char();
    uart2_write_char(c);                   /* echo so you can see what you type */

    if (c == '\r' || c == '\n')
    {
        cmd[idx] = '\0';                   /* finish the string */
        if (strcmp(cmd, "on") == 0)       GPIOA_ODR |=  (1 << 5);
        else if (strcmp(cmd, "off") == 0) GPIOA_ODR &= ~(1 << 5);
        else uart2_write_string("\r\nunknown\r\n");
        idx = 0;
    }
    else if (idx < sizeof(cmd) - 1)
    {
        cmd[idx++] = c;
    }
}
```

---

## Part 7: Polling vs Interrupt vs DMA

| Method | How | CPU cost | Use when |
|---|---|---|---|
| **Polling** | Loop checking RXNE/TXE | CPU busy waiting | Simple programs, learning |
| **Interrupt** | ISR runs when a byte arrives/leaves | Small cost per byte | Most real firmware |
| **DMA** | Hardware moves bytes to/from RAM | Nearly zero | High speed, large blocks |

Polling problem: while the CPU waits in `while(!(SR & RXNE))`, it can do nothing else. If a byte arrives while the CPU is busy elsewhere and a second one arrives before you read the first → **Overrun Error**, data lost.

---

## Part 8: Interrupt‑driven receive

### 8.1 Setup

```c
/* in uart2_init(), after enabling: */
USART2_CR1 |= USART_CR1_RXNEIE;            /* interrupt on byte received */

/* NVIC: USART2 is IRQ 38 -> ISER1 bit (38 - 32) = 6 */
NVIC_ISER1 |= (1 << 6);
```

### 8.2 The ISR

The name must match the startup file exactly: **`USART2_IRQHandler`**.

```c
volatile char rx_byte;
volatile uint8_t rx_flag = 0;

void USART2_IRQHandler(void)
{
    if (USART2_SR & USART_SR_RXNE)
    {
        rx_byte = (char)USART2_DR;         /* reading DR clears RXNE */
        rx_flag = 1;
    }
}
```

Then `main()` checks `rx_flag` whenever it likes. The CPU is free the rest of the time.

**Rules for a UART ISR:**
- Keep it short. Never call `uart2_write_string()` or delay inside it.
- Always read DR (that is what clears the interrupt). If you don't, the ISR fires forever.
- Shared variables must be `volatile`.

### 8.3 Circular (ring) buffer

A single `rx_byte` is lost if a second byte arrives before main reads it. The standard fix is a **ring buffer**: the ISR writes bytes in, main reads them out.

```c
#define RX_BUF_SIZE 64                     /* power of 2 */

volatile char    rx_buf[RX_BUF_SIZE];
volatile uint8_t rx_head = 0;              /* ISR writes here */
volatile uint8_t rx_tail = 0;              /* main reads here */

void USART2_IRQHandler(void)
{
    if (USART2_SR & USART_SR_RXNE)
    {
        char c = (char)USART2_DR;
        uint8_t next = (rx_head + 1) & (RX_BUF_SIZE - 1);
        if (next != rx_tail)               /* not full */
        {
            rx_buf[rx_head] = c;
            rx_head = next;
        }
        /* else: buffer full, byte dropped */
    }
}

int uart2_available(void)
{
    return rx_head != rx_tail;
}

char uart2_getc(void)
{
    while (rx_head == rx_tail);            /* wait for data */
    char c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) & (RX_BUF_SIZE - 1);
    return c;
}
```

Why `& (SIZE - 1)` instead of `% SIZE`? For a power‑of‑2 size they give the same result, and AND is one CPU instruction while modulo is a slow division.

**This ring buffer is one of the most common embedded interview coding questions.** Be able to write it from memory.

### 8.4 Interrupt‑driven transmit (brief)

Set TXEIE, and in the ISR write the next byte from a TX buffer each time TXE fires. When the buffer is empty, clear TXEIE. This lets `printf` return instantly instead of waiting for every byte.

---

## Part 9: DMA with UART (advanced)

DMA (Direct Memory Access) copies data between RAM and the USART DR without the CPU.

On the STM32F103: **DMA1 Channel 6 = USART2_RX**, **DMA1 Channel 7 = USART2_TX**.

Steps for TX:
1. Enable DMA1 clock (AHBENR bit 0).
2. Set channel 7: peripheral address = &USART2_DR, memory address = your buffer, count = length, direction memory→peripheral, memory increment on.
3. Set `DMAT` in USART2_CR3.
4. Enable the channel. The whole string goes out with zero CPU work. The DMA "transfer complete" interrupt tells you when done.

RX with DMA + IDLE line interrupt is the standard way to receive variable‑length packets at high speed (the IDLE flag fires when the line goes quiet after a packet).

You will do DMA properly in Week 5.

---

## Part 10: Errors and how to handle them

| Flag | Cause | Fix |
|---|---|---|
| **FE** framing | Stop bit read as 0 → baud mismatch or noise | Check baud on both sides |
| **ORE** overrun | Byte arrived while the previous one was unread | Use interrupts or DMA, read DR faster |
| **NE** noise | Glitch inside a bit | Shorter wires, better ground |
| **PE** parity | Parity mismatch | Check parity settings |

**Clearing the error flags on the F103:** read **SR**, then read **DR**, in that order. Reading DR alone does not clear ORE. In an ISR always check ORE too, otherwise an overrun can lock the receiver permanently:

```c
if (USART2_SR & (1 << 3))   /* ORE */
{
    (void)USART2_SR;
    (void)USART2_DR;         /* clears ORE */
}
```

---

## Part 11: printf over UART (retargeting)

With the GCC toolchain in STM32CubeIDE, `printf` calls a low‑level function `_write`. Define it yourself and `printf` goes to the UART:

```c
int _write(int file, char *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        uart2_write_char(ptr[i]);
    }
    return len;
}
```

Then `printf("Temp = %d\r\n", temp);` works. Notes:
- `printf` with `%f` needs the "use float with printf" linker option and adds a lot of code size. Prefer integers on small MCUs.
- `printf` is slow and uses stack. Never call it inside an ISR.

---

## Part 12: Advanced features (know what they are)

### 12.1 Hardware flow control (RTS/CTS)
Two extra wires. The receiver pulls **RTS** low when it has buffer space; the sender checks **CTS** before sending. Prevents overrun at high baud rates. Enable with RTSE/CTSE in CR3. Pins for USART2: PA1 (RTS), PA0 (CTS).

### 12.2 Software flow control (XON/XOFF)
Instead of wires, the receiver sends special bytes (0x13 = XOFF "stop", 0x11 = XON "go"). Only for text data.

### 12.3 9‑bit mode and multiprocessor communication
Set M = 1 for 9 data bits. The 9th bit can mark "this byte is an address". Slaves in mute mode (RWU) wake up only when they see their address. Used for RS‑485 multi‑drop networks.

### 12.4 Half duplex (single wire)
HDSEL in CR3: TX and RX share one pin. Used with some sensors and for Smart Card.

### 12.5 Synchronous mode
CLKEN in CR2 outputs a clock on the CK pin: the USART then behaves like an SPI master. Rarely used.

### 12.6 LIN (Local Interconnect Network)
Automotive low‑cost bus built on UART (one master, up to 16 slaves, 19.2 kbps). LINEN bit adds break detection. Relevant to your automotive background: LIN is used for window lifts, mirrors, seat controls and sometimes TPMS receivers.

### 12.7 IrDA and Smartcard
Special modes in the same peripheral: infrared remote style encoding, and ISO 7816 smart cards. Just know they exist.

### 12.8 Break frame
A long LOW (longer than a whole frame). Used by LIN to start a message and by some bootloaders. SBK bit sends one; LBD flag detects one.

### 12.9 Auto‑baud
The F103 does not have it. Newer STM32s (F0/F3/L4/G4) can measure the first received byte and set BRR automatically.

---

## Part 13: Debugging checklist

**Nothing appears on the terminal**
1. Correct COM port chosen? (Device Manager → STLink Virtual COM Port)
2. Same baud rate on both sides?
3. USART2 clock enabled (APB1ENR bit 17)?
4. GPIOA clock and AFIO clock enabled?
5. PA2 set to **alternate function** push‑pull (0xB), not normal output (0x2)?
6. UE, TE set in CR1?
7. Terminal set to 8N1, no flow control?

**Garbage characters**
- Baud rate wrong (check BRR math and which clock feeds the USART: APB1 vs APB2).
- If you changed the system clock (PLL), PCLK1 is no longer 8 MHz.

**Receiving works once, then stops**
- Overrun error (ORE) locked the receiver. Read SR then DR to clear.

**Logic analyzer check**
- Connect a probe to PA2 and GND, set the analyzer's UART decoder to your baud rate. You should see the start bit, 8 data bits LSB first, stop bit. Measure one bit width: 104 µs = 9600 baud. This is the fastest way to prove whether the MCU or the PC is at fault.

---

## Part 14: Interview questions (answer from memory)

1. **What does asynchronous mean in UART?** No shared clock wire; both sides use an agreed baud rate.
2. **Draw the UART frame for 8N1.** Start (LOW), 8 data bits LSB first, stop (HIGH). 10 bits total.
3. **Why is the start bit LOW?** The idle line is HIGH, so a HIGH→LOW edge clearly marks the beginning of a frame.
4. **What is baud rate? What happens if it mismatches?** Bits per second. Mismatch → garbage characters and framing errors.
5. **How do you calculate BRR for 9600 at 8 MHz?** USARTDIV = 8M/(16×9600) = 52.08 → mantissa 52, fraction 1 → 0x341.
6. **TXE vs TC?** TXE: data register empty, you may write the next byte. TC: last bit has left the wire.
7. **What is overrun and how to avoid it?** New byte arrives before the previous was read; use interrupts/DMA and read promptly.
8. **Polling vs interrupt vs DMA?** See Part 7.
9. **What is a ring buffer and why use it with UART?** FIFO in an array with head/tail indices; lets the ISR store bytes until main is ready.
10. **UART vs SPI vs I2C?** UART: 2 wires, async, point‑to‑point, ~1 Mbps. SPI: 4 wires, sync, master/slave, very fast, CS per device. I2C: 2 wires, sync, multi‑device by address, 100k–400k, needs pull‑ups.
11. **UART vs RS‑232 vs RS‑485?** Same frame, different voltage levels and distance. RS‑485 is differential and multi‑drop.
12. **What is parity?** Extra bit making the count of 1s even/odd, detects single‑bit errors.
13. **Why `volatile` on the register and on `rx_flag`?** Hardware/ISR can change them outside the compiler's view.
14. **Can you call printf inside an ISR?** Should not: too slow, not reentrant, uses stack.
15. **How does the receiver sample a bit?** Waits for the start edge, moves to bit centre, samples 16× per bit and majority‑votes the 3 middle samples.
16. **What is flow control?** RTS/CTS wires (or XON/XOFF bytes) so a receiver can pause the sender.
17. **What is LIN?** Automotive single‑wire bus built on UART frames, master/slave, 19.2 kbps.

---

## Part 15: Your Week 1 build list

Do these in order. Push to GitHub after each one.

| Day | Task | Done when |
|---|---|---|
| 1 | `uart2_init` + send "Hello from STM32\r\n" once at startup | Text appears in Tera Term at 9600 |
| 2 | Echo program (Part 6.5) + `uart2_write_uint` | Typing shows characters back; number 12345 prints correctly |
| 3 | Command parser: `on` / `off` controls the LED (Part 6.7) | LED obeys the keyboard |
| 4 | Change to 115200 baud; recalculate BRR by hand first | Still works at 115200 |
| 5 | Interrupt‑driven RX with `rx_flag` (Part 8.2) | Same echo, but main loop blinks the LED at the same time |
| 6 | Ring buffer (Part 8.3) | Paste a 30‑character line into the terminal; all characters echo correctly |
| 7 | Capture PA2 on the logic analyzer, measure a bit; write README | You can show a screenshot of the frame for 'A' and explain each bit |

Bonus: add `_write` so `printf` works (Part 11).

---

## Quick reference card

```
USART2 base      0x40004400       APB1ENR bit 17      IRQ 38 (ISER1 bit 6)
TX PA2  CRL[11:8]  = 0xB (AF push-pull 50MHz)
RX PA3  CRL[15:12] = 0x4 (floating input)
BRR = PCLK1 / baud            (8 MHz: 9600 -> 0x341, 115200 -> 0x45)
CR1 = UE(13) | TE(3) | RE(2)  = 0x200C     + RXNEIE(5) for RX interrupt
send: wait TXE(7) -> write DR  ;  recv: wait RXNE(5) -> read DR
errors: ORE(3) FE(1) NE(2) PE(0) -> read SR then DR to clear
```
