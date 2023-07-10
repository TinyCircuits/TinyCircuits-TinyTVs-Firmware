// RP2040 PIO spi implementation, adapted from Bodmer's TFT_eSPI

static uint32_t pull_stall_mask;
static int8_t pio_sm;
static int32_t dma_tx_channel;
static dma_channel_config dma_tx_config;
static PIO tft_pio;
static uint32_t program_offset;
static uint32_t pio_instr_jmp8;
static uint32_t pio_instr_fill;
static uint32_t pio_instr_addr;
static uint32_t pio_instr_set_dc;
static uint32_t pio_instr_clr_dc;

#define tft_io_wrap_target 27
#define tft_io_wrap 31

#define tft_io_offset_start_8 0u
#define tft_io_offset_set_addr_window 3u
#define tft_io_offset_block_fill 17u
#define tft_io_offset_start_tx 27u

#define TX_FIFO  tft_pio->txf[pio_sm]

#define write8(C) tft_pio->sm[pio_sm].instr = pio_instr_jmp8; TX_FIFO = (C); stallWait()
#define write16(C) fifoWait(1); TX_FIFO = (C)
static inline void stallWait()
{
  tft_pio->fdebug = pull_stall_mask; while (!(tft_pio->fdebug & pull_stall_mask));
}

static inline void clearDC()
{
  stallWait(); tft_pio->sm[pio_sm].instr = pio_instr_clr_dc;
}

static inline void setDC()
{
  tft_pio->sm[pio_sm].instr = pio_instr_set_dc;
}

#define swap(x,y) x ^= y; y ^= x; x ^= y
#define CS_L sio_hw->gpio_clr = (1ul << csPin)
#define CS_H stallWait(); sio_hw->gpio_set = (1ul << csPin)

static inline void fifoWait(const int level)
{
  while (((tft_pio->flevel >> (pio_sm * 8)) & 0x000F) > (8-level)){}
}

static inline void writeCommand(const uint8_t c)
{
  CS_L; clearDC();
  write8(c);
  setDC(); CS_H;
}

static inline void writeData(const uint8_t d)
{
  CS_L; setDC();
  write8(d);
  CS_L; CS_H;
}

static inline void pushBlock16(const uint16_t color, const uint32_t len)
{
  if (len > 0) {
    stallWait();
    tft_pio->sm[pio_sm].instr = pio_instr_fill;
    TX_FIFO = color;
    TX_FIFO = len-1; // Decrement first as PIO sends n+1
  }
}

static inline void pushBlock8(const uint8_t color, const uint32_t len)
{
  if(len & 1)
  {
    stallWait();
    write8(color);
  }
  stallWait();
  pushBlock16(color | (color << 8), len >> 1);
}

static const uint16_t tft_io_program_instructions[] = {
    0x90a0, //  0: pull   block           side 0
    0x6019, //  1: out    pins, 25
    0x181e, //  2: jmp    30              side 1
    0xf022, //  3: set    x, 2            side 0
    0xe000, //  4: set    pins, 0
    0x90a0, //  5: pull   block           side 0
    0x6019, //  6: out    pins, 25
    0xb842, //  7: nop                    side 1
    0x7001, //  8: out    pins, 1         side 0
    0x18e8, //  9: jmp    !osre, 8        side 1
    0xf001, // 10: set    pins, 1         side 0
    0x003b, // 11: jmp    !x, 27
    0x80a0, // 12: pull   block
    0x7001, // 13: out    pins, 1         side 0
    0x18ed, // 14: jmp    !osre, 13       side 1
    0x1044, // 15: jmp    x--, 4          side 0
    0x001b, // 16: jmp    27
    0x90a0, // 17: pull   block           side 0
    0xa027, // 18: mov    x, osr
    0x80a0, // 19: pull   block
    0xa047, // 20: mov    y, osr
    0xb0e1, // 21: mov    osr, x          side 0
    0x7011, // 22: out    pins, 17        side 0
    0xb842, // 23: nop                    side 1
    0x7001, // 24: out    pins, 1         side 0
    0x18f8, // 25: jmp    !osre, 24       side 1
    0x1095, // 26: jmp    y--, 21         side 0
            //     .wrap_target
    0x90a0, // 27: pull   block           side 0
    0x7011, // 28: out    pins, 17        side 0
    0xb842, // 29: nop                    side 1
    0x7001, // 30: out    pins, 1         side 0
    0x18fe, // 31: jmp    !osre, 30       side 1
            //     .wrap
};

static const struct pio_program tft_io_program = {
    .instructions = tft_io_program_instructions,
    .length = 32,
    .origin = -1,
};

static inline pio_sm_config tft_io_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + 27, offset + 31);
    sm_config_set_sideset(&c, 2, true, false);
    return c;
}

static void pioinit(uint32_t clock_freq) {

  // Find a free SM on one of the PIO's
  tft_pio = pio0;

  /*
  pio_sm = pio_claim_unused_sm(tft_pio, false); // false means don't panic
  // Try pio1 if SM not found
  if (pio_sm < 0) {
    tft_pio = pio1;
    pio_sm = pio_claim_unused_sm(tft_pio, true); // panic this time if no SM is free
  }
  */

  // Find enough free space on one of the PIO's
  tft_pio = pio0;
  if (!pio_can_add_program(tft_pio, &tft_io_program)) {
    tft_pio = pio1;
    if (!pio_can_add_program(tft_pio, &tft_io_program)) {
      //Serial.println("No room for PIO program!");
      return;
    }
  }

  pio_sm = pio_claim_unused_sm(tft_pio, false);

  // Load the PIO program
  program_offset = pio_add_program(tft_pio, &tft_io_program);

  // Associate pins with the PIO
  pio_gpio_init(tft_pio, dcPin);
  pio_gpio_init(tft_pio, sclkPin);
  pio_gpio_init(tft_pio, mosiPin);

  // Configure the pins to be outputs
  pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, dcPin, 1, true);
  pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, sclkPin, 1, true);
  pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, mosiPin, 1, true);

  // Configure the state machine
  pio_sm_config c = tft_io_program_get_default_config(program_offset);

  sm_config_set_set_pins(&c, dcPin, 1);
  // Define the single side-set pin
  sm_config_set_sideset_pins(&c, sclkPin);
  // Define the pin used for data output
  sm_config_set_out_pins(&c, mosiPin, 1);
  // Set clock divider, frequency is set up to 5% faster than specified, or next division down
  uint16_t clock_div = 0.95 + clock_get_hz(clk_sys) / (clock_freq * 2.0); // 2 cycles per bit
  sm_config_set_clkdiv(&c, clock_div);
  // Make a single 8 words FIFO from the 4 words TX and RX FIFOs
  sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
  // The OSR register shifts to the left, sm designed to send MS byte of a colour first, autopull off
  sm_config_set_out_shift(&c, false, false, 0);
  // Now load the configuration
  pio_sm_init(tft_pio, pio_sm, program_offset + 27, &c);

  // Start the state machine.
  pio_sm_set_enabled(tft_pio, pio_sm, true);

  // Create the pull stall bit mask
  pull_stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + pio_sm);

  // Create the assembler instruction for the jump to byte send routine
  pio_instr_jmp8  = pio_encode_jmp(program_offset + tft_io_offset_start_8);
  pio_instr_fill  = pio_encode_jmp(program_offset + tft_io_offset_block_fill);
  pio_instr_addr  = pio_encode_jmp(program_offset + tft_io_offset_set_addr_window);
  pio_instr_set_dc = pio_encode_set((pio_src_dest)0, 1);
  pio_instr_clr_dc = pio_encode_set((pio_src_dest)0, 0);
}
