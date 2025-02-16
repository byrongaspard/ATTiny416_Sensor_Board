/*
 * File:   main.c
 * Author: byrongaspard
 *
 * Created on February 15, 2025, 3:23 PM
 */

// Clock Speed, including the Peripheral Clock.
// F_CPU must be defined before including the delay.h library for the delay calls to work
#define F_CPU   3300000UL
                
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/xmega.h>


// General Bit Getting/Setting/Clearing utilities
#define GET_BIT(p,n) ((p >> n) & 0x1)
#define SET_BIT(p,n) ((p) |= (0x1 << (n)))
#define CLR_BIT(p,n) ((p) &= ~((0x1) << (n)))

// USART Definitions
#define USART_S             (16UL)      
#define USART_REG_FRACT     (64UL)    
#define BAUD_RATE           (9600UL)
#define BAUD_REG_VAL        ( ((float)USART_REG_FRACT * F_CPU)/((float)USART_S * BAUD_RATE) )

// W25X20L Info
#define W25X20L_MFR_ID  (0xEF)
#define W25X20L_DEV_ID  (0x11)


// W25X20L SPI Flash Commands
#define W25X20L_READ_MFR_DEV_ID     (0x90)

// SPI helper commands
#define ASSERT_SELECT       (0)
#define DEASSERT_SELECT     (1)


// Function prototypes
void initialize_clock(void);
void configure_alt_pinning(void);
void configure_data_direction(void);
void configure_uart(void);
void configure_spi(void);

void set_spi_cs(uint8_t val);
uint8_t spi_transfer(uint8_t byte);
void get_spi_device_id(void);


void send_uart_byte(uint8_t byte);
void send_uart_string(char* str);
void send_uart_ascii_hex_byte(uint8_t byte);

void main(void) 
{
    
    initialize_clock();
    configure_alt_pinning();
    configure_data_direction();
    configure_uart();
    configure_spi();

    get_spi_device_id();
   
    while(1)
    {
        send_uart_string("Hello!\n");
        _delay_ms(1000);
    }
    return;
}

void initialize_clock(void)
{
    
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
    
    set_spi_cs(ASSERT_SELECT);
    
    spi_transfer(W25X20L_READ_MFR_DEV_ID);
    spi_transfer(0x00);
    spi_transfer(0x00);
    spi_transfer(0x00);
    
    mfr_id = spi_transfer(0x00);
    dev_id = spi_transfer(0x00);
    
    set_spi_cs(DEASSERT_SELECT);
    
    if(mfr_id != W25X20L_MFR_ID || dev_id != W25X20L_DEV_ID)
    {
        send_uart_string("WARNING: detected illegal flash chip");
        send_uart_string("mfr_id is ");
        send_uart_ascii_hex_byte(mfr_id);
        send_uart_string("\n");

        send_uart_string("device_id is ");
        send_uart_ascii_hex_byte(dev_id);
        send_uart_string("\n");
    }
    else
    {
        send_uart_string("Detected W25X20L Flash Chip.\n");
    }
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
    send_uart_string("0x");
    
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