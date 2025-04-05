/*
 * File:   main.c
 * Author: byrongaspard
 *
 * Created on February 15, 2025, 3:23 PM
 * 
 * This test program exercises the interfacing hardware on the custom ATTiny416 
 * development board. 
 * It initializes an external serial communication interface to the FTDI230 chip.
 * Additionally, it exercises the external W25X20L flash chip.
 * 
 * Connecting PA6 to ground will cause the ATTiny416 to dump the contents of
 * the W25X20L flash to the UART interface. 
 * Connecting PA7 to ground will cause the ATTiny416 to erase the W25X20L flash.
 * If both PA6 and PA7 are left floating, the ATTiny416 will look for the beginning
 * of uninitialized flash and begin recording data.
 * 
 * TODO: Separate code out into logical *.c and *.h files. 
 * 
 */

// Clock Speed, including the Peripheral Clock.
// Trap for new players: F_CPU must be defined before including the delay.h library for the delay calls to work.
#define F_CPU   3300000UL
                
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/xmega.h>

#define TRUE                (1)
#define FALSE               (0)

// General Bit Getting/Setting/Clearing utilities
#define GET_BIT(p,n) ((p >> n) & 0x1)
#define SET_BIT(p,n) ((p) |= (0x1 << (n)))
#define CLR_BIT(p,n) ((p) &= ~((0x1) << (n)))

// USART Definitions
#define USART_S             (16UL)      
#define USART_REG_FRACT     (64UL)    
#define BAUD_RATE           (115200UL)
#define BAUD_REG_VAL        ( ((float)USART_REG_FRACT * F_CPU)/((float)USART_S * BAUD_RATE) )

// W25X20L Info
#define W25X20L_MFR_ID              (0xEF)
#define W25X20L_DEV_ID              (0x11)
#define W25X20L_START_ADDR          (0x000000)
#define W25X20L_END_ADDR            (0x03FFFF)
#define W25X20L_PAGE_BYTES          (256)
#define W25X20L_STATUS_REG_WEN_BIT  (1UL)
#define W25X20L_STATUS_REG_BUSY_BIT (0UL)
#define W25X20L_NUM_PAGES           ((W25X20L_END_ADDR + 1UL) / W25X20L_PAGE_BYTES)
#define W25X20L_CHIP_ERASE_TIME_MS  (2500UL)

// W25X20L SPI Flash Commands
#define W25X20L_READ_MFR_DEV_ID     (0x90)
#define W25X20L_WRITE_ENABLE        (0x06)
#define W25X20L_READ_STATUS_REG     (0x05)
#define W25X20L_CHIP_ERASE          (0xC7)
#define W25X20L_READ_DATA           (0x03) 
#define W25X20L_PAGE_PROGRAM        (0x02)

// SPI helper commands
#define ASSERT_SELECT       (0)
#define DEASSERT_SELECT     (1)

// Global Variables
volatile uint32_t cur_spi_write_addr = 0UL;
volatile uint8_t spi_write_permitted = FALSE;

// Function prototypes
void initialize_clock(void);
void configure_alt_pinning(void);
void configure_data_direction(void);
void configure_uart(void);
void configure_spi(void);
void set_spi_cs(uint8_t val);
uint8_t spi_transfer(uint8_t byte);
void get_spi_device_id(void);

void W25X20L_write_enable(void);
void W25X20L_chip_erase(void);
void W25X20L_find_uninitialized_memory(void);
void W25X20L_write_data(uint8_t *data, uint8_t num_bytes);
void W25X20L_dump_flash_to_uart(void);
uint8_t W25X20L_get_status_reg(void);

void send_uart_byte(uint8_t byte);
void send_uart_string(char* str);
void send_uart_ascii_hex_byte(uint8_t byte);

void main(void) 
{
    // Test recording array
    uint8_t test_arr[] = {1, 2, 3, 99};
    
    // Initialize everything
    initialize_clock();
    configure_alt_pinning();
    configure_data_direction();
    configure_uart();
    configure_spi();

    // Check that the W25X20L is detected
    get_spi_device_id();

   
    if(!GET_BIT(PORTA.IN, 6))
    {
        send_uart_string("PA6 is LOW. Dumping Flash\n");
        W25X20L_dump_flash_to_uart();
        send_uart_string("\n\nFlash dump complete! Reboot required.\n");
        while(1);
    }
    
    if(!GET_BIT(PORTA.IN, 7))
    {
        send_uart_string("PA7 is LOW. Erasing Flash. \n");
        W25X20L_chip_erase();
        send_uart_string("Reboot required. \n");
        while(1);
    }
    
    W25X20L_find_uninitialized_memory();
    
    while(1)
    {
        if(GET_BIT(PORTA.IN, 6))
        {
            W25X20L_write_enable();
            //send_uart_string("PA6 is HIGH. Recording Data. \n");
            W25X20L_write_data(test_arr, sizeof(test_arr)/sizeof(test_arr[0]));
            test_arr[0]++;
        }
        
        //_delay_ms(1000);
    }
    return;
}

void initialize_clock(void)
{
    // NOP...
    return;
}


void configure_alt_pinning(void)
{
    SET_BIT(PORTMUX.CTRLB, 0); // Configure USART0 to use alternate pinning.
    SET_BIT(PORTMUX.CTRLB, 2); // Configure SPI0 to use alternate pinning.
    return;
}

void configure_data_direction(void)
{
    // USART Configuration
        SET_BIT(PORTA.OUT, 1);  // TxD is on PA1. Initialize HIGH (idle).
        SET_BIT(PORTA.DIR, 1);  // TxD is on PA1. Initialize as output.
        CLR_BIT(PORTA.DIR, 2);  // RxD is on PA2. Initialize as input.

    // SPI Configuration
        CLR_BIT(PORTC.OUT, 0);  // CLK is on PC0. Initialize low.
        SET_BIT(PORTC.DIR, 0);  // CLK is on PC0. Initialize as output.

        CLR_BIT(PORTC.DIR, 1);  // MISO is on PC1. Initialize as input.

        CLR_BIT(PORTC.OUT, 2);  // MOSI is on PC2. Initialize low. 
        SET_BIT(PORTC.DIR, 2);  // MOSI is on PC2. Initialize as output. 

        SET_BIT(PORTC.OUT, 3);  // CS/SS is on PC3. Initialize high.
        SET_BIT(PORTC.DIR, 3);  // CS/SS is on PC3. Configure as output. 
    
    // LED Configuration
        CLR_BIT(PORTA.OUT, 3); // LED is on PA3. Initialize ON.
        SET_BIT(PORTA.DIR, 3); // LED is on PA3. Configure as output.
        
    // GPIO Configuration
        CLR_BIT(PORTA.DIR, 7);      // GPIO input on PA7
        SET_BIT(PORTA.PIN7CTRL, 3); // Enable pull-up resistor
       
        CLR_BIT(PORTA.DIR, 6);      // GPIO input on PA6
        SET_BIT(PORTA.PIN6CTRL, 3); // Enable pull-up resistor

    return;
}

void configure_uart(void)
{    
    // Set the baud rate (in the USARTn.BAUD register) and frame format.
    USART0.BAUD = (uint16_t)BAUD_REG_VAL;
   
    // Enable the transmitter and the receiver.
    SET_BIT(USART0.CTRLB, 7); // Enable the receiver
    SET_BIT(USART0.CTRLB, 6); // Enable the transmitter
    
    // Wait until the transmitter is ready by checking DREIF
    while(!GET_BIT(USART0.STATUS, 5)); 
    
    send_uart_string("\n\nBooting...\n");
    send_uart_string("UART Initialized.\n");
    
    return;
}

void configure_spi(void)
{
    CLR_BIT(SPI0.CTRLA, 6); // MSb is sent first.
    SET_BIT(SPI0.CTRLA, 5); // Configure as Host/Master
    SET_BIT(SPI0.CTRLA, 4); // Double clock speed
    SET_BIT(SPI0.CTRLA, 0); // Enable SPI Interface
    
    send_uart_string("SPI Initialized.\n");
    
    return;
}

void set_spi_cs(uint8_t val)
{
    if(val == ASSERT_SELECT)
    {
        CLR_BIT(PORTC.OUT, 3);  // CS/SS is on PC3. Set LOW to select chip.
    }
    else
    {
        SET_BIT(PORTC.OUT, 3);  // CS/SS is on PC3. Set LOW to select chip.
    }
}

uint8_t spi_transfer(uint8_t byte)
{
    SPI0.DATA = byte; // Send the data
    while(!(SPI0.INTFLAGS & SPI_IF_bm)); // Loop until the transmit is complete
    return SPI0.DATA;
}

void get_spi_device_id(void)
{
    uint8_t mfr_id = 0x00;
    uint8_t dev_id = 0x00;
    
    //TODO: Replace this with the JEDEC ID call.
    
    // Wait until the chip is ready.
    while(GET_BIT(W25X20L_get_status_reg(), W25X20L_STATUS_REG_BUSY_BIT)) _delay_us(1);
    
    set_spi_cs(ASSERT_SELECT);
    
    /* Send the command IAW W25X20L datasheet. */
    spi_transfer(W25X20L_READ_MFR_DEV_ID);
    spi_transfer(0x00);
    spi_transfer(0x00);
    spi_transfer(0x00);
    
    mfr_id = spi_transfer(0x00);
    dev_id = spi_transfer(0x00);
    
    set_spi_cs(DEASSERT_SELECT);
    
    // Verify the mfr and dev IDs are what is expected.
    if(mfr_id != W25X20L_MFR_ID || dev_id != W25X20L_DEV_ID)
    {
        send_uart_string("WARNING: detected illegal flash chip");
        send_uart_string("mfr_id is 0x");
        send_uart_ascii_hex_byte(mfr_id);
        send_uart_string("\n");

        send_uart_string("device_id is 0x");
        send_uart_ascii_hex_byte(dev_id);
        send_uart_string("\n");
        // TODO: Add error handling here so we don't try to write if we're not 
        // talking to the right chip.
    }
    else
    {
        send_uart_string("Detected W25X20L Flash Chip.\n");
    }
}

uint8_t W25X20L_get_status_reg(void)
{
    uint8_t status_reg = 0x00;
    
    // Read the status register.
    set_spi_cs(ASSERT_SELECT);
    spi_transfer(W25X20L_READ_STATUS_REG);
    status_reg = spi_transfer(0x00);
    set_spi_cs(DEASSERT_SELECT);
    
    return status_reg;
}

void W25X20L_write_enable(void)
{
    // Wait until the chip is ready.
    while(GET_BIT(W25X20L_get_status_reg(), W25X20L_STATUS_REG_BUSY_BIT)) _delay_us(1);
    
    // Send Write Enable command.
    set_spi_cs(ASSERT_SELECT);
    spi_transfer(W25X20L_WRITE_ENABLE);
    set_spi_cs(DEASSERT_SELECT);
    
    if(!GET_BIT(W25X20L_get_status_reg(), W25X20L_STATUS_REG_WEN_BIT))
    {
        send_uart_string("ERROR: Write enable failed.\n");
        spi_write_permitted = FALSE;
    }
    
    return;
}

void W25X20L_chip_erase(void)
{
    // Enable writes.
    W25X20L_write_enable();
    
    // Wait until the chip is ready.
    while(GET_BIT(W25X20L_get_status_reg(), W25X20L_STATUS_REG_BUSY_BIT)) _delay_us(1);
    
    // Send the erase command.
    set_spi_cs(ASSERT_SELECT);
    spi_transfer(W25X20L_CHIP_ERASE);
    set_spi_cs(DEASSERT_SELECT);

    // Poll the status register until the erase is complete as indicated by the BUSY bit.
     while(GET_BIT(W25X20L_get_status_reg(), W25X20L_STATUS_REG_BUSY_BIT))
     {
         _delay_ms(100);
        send_uart_string("Erasing...\n");
     }
    
    send_uart_string("W25X20L Erase Complete.\n");
}

/*
 * This through iterates through flash until it finds 16 consecutive bytes of 0xFF.
 * The memory address is then aligned to the nearest 16-byte boundary which is considered
 * to be the start of uninitialized memory.
 */
void W25X20L_find_uninitialized_memory(void)
{
    uint8_t consec_uninit_bytes = 0;
    uint32_t cur_addr = W25X20L_START_ADDR;
    uint32_t start_addr = W25X20L_START_ADDR;
    
    // Wait until the chip is ready.
    while(GET_BIT(W25X20L_get_status_reg(), W25X20L_STATUS_REG_BUSY_BIT)) _delay_us(1);
    
    // Send the Read Data command IAW W25X20L datasheet
    // Start reading from the beginning of memory
    set_spi_cs(ASSERT_SELECT);
    spi_transfer(W25X20L_READ_DATA); // Send the read command.
    spi_transfer((cur_addr >> 16) & 0xFF);  // Shift upper byte of address
    spi_transfer((cur_addr >> 8) & 0xFF);   // Shift middle byte of address
    spi_transfer((cur_addr >> 0) & 0xFF);   // Shift lower byte of address
    
    // Loop while there is still data to read
    while(cur_addr <= W25X20L_END_ADDR)
    {
        if(spi_transfer(0x00) == 0xFF)
        {
            consec_uninit_bytes++;
            
            // If the start of uninitialized flash has been found, find the nearest
            // 16-byte aligned address.
            if(consec_uninit_bytes == 16)
            {
                start_addr = cur_addr & 0xFFFFFFF0;
                
                send_uart_string("Start of memory is at 0x");
                send_uart_ascii_hex_byte(((start_addr) >> 16) & 0xFF);
                send_uart_ascii_hex_byte(((start_addr) >> 8) & 0xFF);
                send_uart_ascii_hex_byte(((start_addr) >> 0) & 0xFF);
                send_uart_string("\n");
                
                cur_spi_write_addr = start_addr;
                spi_write_permitted = TRUE;
                
                return;
            }
        }
        else
        {
            consec_uninit_bytes = 0;
        }
        cur_addr ++;
    }
    
    send_uart_string("Could not find start of memory.");
    set_spi_cs(DEASSERT_SELECT);
}

void W25X20L_write_data(uint8_t *data, uint8_t num_bytes)
{    
    // Check that there is enough memory to write the data.
    if((cur_spi_write_addr + num_bytes) > W25X20L_END_ADDR + 1)
    {
        send_uart_string("Aborting W25X20L write... not enough memory left\n");
        while(1);
        return;
    }
    // CHeck that the write does not cross page boundaries.
    if(((cur_spi_write_addr % W25X20L_PAGE_BYTES) + num_bytes) > W25X20L_PAGE_BYTES)
    {
        // Feature: Make this function recursive to support multi-page writes.
        send_uart_string("Aborting W25X20L write... the requested write crosses page boundaries. Not supported.\n");
        return;
    }
    
    /* Enable writes */
    W25X20L_write_enable();
    
    if(spi_write_permitted == TRUE)
    {
        /* Wait until the chip is ready */
        while(GET_BIT(W25X20L_get_status_reg(), W25X20L_STATUS_REG_BUSY_BIT)) _delay_us(1);

        set_spi_cs(ASSERT_SELECT);
        spi_transfer(W25X20L_PAGE_PROGRAM); // Send the program command.
        spi_transfer((cur_spi_write_addr >> 16) & 0xFF);  // Shift upper byte of address
        spi_transfer((cur_spi_write_addr >> 8) & 0xFF);   // Shift middle byte of address
        spi_transfer((cur_spi_write_addr >> 0) & 0xFF);   // Shift lower byte of address
        for(uint8_t i = 0; i < num_bytes; i++)
        {
            spi_transfer(data[i]);
        }
        set_spi_cs(DEASSERT_SELECT);

        _delay_ms(10);

        cur_spi_write_addr += num_bytes;
    }
    return;
}

void W25X20L_dump_flash_to_uart(void)
{
    uint32_t cur_addr = W25X20L_START_ADDR;
    uint8_t cur_byte = 0x00;
    uint8_t checksum = 0x00;
    
    /* Wait until the chip is ready */
    while(GET_BIT(W25X20L_get_status_reg(), W25X20L_STATUS_REG_BUSY_BIT)) _delay_us(1);
  
    // Dump one page at a time
    for(uint32_t cur_page = 0; cur_page < W25X20L_NUM_PAGES; cur_page++)
    {
        cur_addr = W25X20L_START_ADDR + (cur_page * W25X20L_PAGE_BYTES);
        
        set_spi_cs(ASSERT_SELECT);
        spi_transfer(W25X20L_READ_DATA); // Send the read command.
        spi_transfer((cur_addr >> 16) & 0xFF);  // Shift upper byte of address
        spi_transfer((cur_addr >> 8) & 0xFF);   // Shift middle byte of address
        spi_transfer((cur_addr >> 0) & 0xFF);   // Shift lower byte of address
        
        for(uint32_t cur_page_byte = 0; cur_page_byte < W25X20L_PAGE_BYTES; cur_page_byte ++)
        {
            if (((cur_page_byte) % 16) == 0 && cur_page_byte < W25X20L_PAGE_BYTES)
            {
                send_uart_string("\n");
                send_uart_string("0x");
                send_uart_ascii_hex_byte(((cur_addr + cur_page_byte) >> 16) & 0xFF);
                send_uart_ascii_hex_byte(((cur_addr + cur_page_byte) >> 8) & 0xFF);
                send_uart_ascii_hex_byte(((cur_addr + cur_page_byte) >> 0) & 0xFF);
                send_uart_string(": ");
            }
            cur_byte = spi_transfer(0x00);
            checksum += cur_byte;
            send_uart_ascii_hex_byte(cur_byte);
        }
        
        set_spi_cs(DEASSERT_SELECT);
    }
    
    send_uart_string("\n\n8-Bit Two's Compliment Checksum: 0x");
    send_uart_ascii_hex_byte(~checksum + 1);
    send_uart_string("\n");
}

/***** USART Helper Functions ******/

void send_uart_byte(uint8_t byte)
{
    // Wait until the transmitter is ready by checking DREIF
    while(!GET_BIT(USART0.STATUS, 5));    
    
    // Send the data
    USART0.TXDATAL = byte;
    
    return;
}

// WARNING: This function requires the caller to provide a null-terminated string.
void send_uart_string(char* str)
{
    uint32_t i = 0;
    while(str[i] != '\0')
    {
        send_uart_byte((uint8_t) str[i]);
        i++;
    }
    
    return;
}

void send_uart_ascii_hex_byte(uint8_t byte)
{
    uint8_t cur_nibble;
    
    cur_nibble = ((byte >> 4) & 0xF);
    
    if(cur_nibble < 0xA) cur_nibble += 48;
    else cur_nibble += 55;
    
    send_uart_byte(cur_nibble);
    
    cur_nibble = ((byte) & 0xF);
    
    if(cur_nibble < 0xA) cur_nibble += 48;
    else cur_nibble += 55;
    
    send_uart_byte(cur_nibble);
                        
    return;
}