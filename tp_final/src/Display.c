#include <Display.h>
#include <stdlib.h>

// IC2 control bits
#define AA      (1 << 2)
#define SI      (1 << 3)
#define STO     (1 << 4)
#define STA     (1 << 5)
#define I2EN    (1 << 6)

static volatile uint8_t  slaveAddress;
static volatile uint8_t* buf;
static volatile uint32_t bufLen;
static volatile uint32_t numTransferido;
static volatile uint32_t i2c0Busy;

uint32_t ignore_data_nack = 1;

uint32_t SendBytes(uint8_t address, uint8_t* buffer, uint32_t length);
void SendData(char data);

void DisplayInit(uint32_t frecuencia, uint8_t intPrioridad) {
    uint32_t pclk, fdiv;

    //Inicializa
    i2c0Busy = 0;
    buf = NULL;
    bufLen = 0;
    numTransferido = 0;

    //Dar power a IC20
    LPC_SC->PCONP |= (1 << 7);

    // Setear PIO0.27 and PIO0.28 en I2C0 SDA and SCK
    LPC_PINCON->PINSEL1 &= ~0x03C00000;
    LPC_PINCON->PINSEL1 |=  0x01400000;

    // Seleccionar clock para I2C0
    LPC_SC->PCLKSEL0 &= ~(3 << 14); // clear bits
    LPC_SC->PCLKSEL0 |=  (1 << 14); // "01" (full speed)
    pclk = SystemCoreClock;

    // clear flags
    LPC_I2C0->I2CONCLR = AA | SI | STO | STA | I2EN;

    // Setear frecuencia I2C0
    // 50% duty cycle
    fdiv = pclk / frecuencia;
    LPC_I2C0->I2SCLL = fdiv >> 1; // fdiv / 2
    LPC_I2C0->I2SCLH = fdiv - (fdiv >> 1); // compensate for odd dividers

    // install interrupt handler
    NVIC_EnableIRQ(I2C0_IRQn);

    // set the priority of the interrupt
    NVIC_SetPriority(I2C0_IRQn, intPrioridad); // '0' is highest

    // enable the I2C (master only)
    LPC_I2C0->I2CONSET = I2EN;
}

void SendCmd(char cmd){

	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (cmd&0xf0);
	data_l = ((cmd<<4)&0xf0);
	data_t[0] = data_u|0x0C;  //en=1, rs=0
	data_t[1] = data_u|0x08;  //en=0, rs=0
	data_t[2] = data_l|0x0C;  //en=1, rs=0
	data_t[3] = data_l|0x08;  //en=0, rs=0

	SendBytes(0x27, data_t, 4);
}

void SendData(char data){

	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (data&0xf0);
	data_l = ((data<<4)&0xf0);
	data_t[0] = data_u|0x0D;  //en=1, rs=1
	data_t[1] = data_u|0x09;  //en=0, rs=1
	data_t[2] = data_l|0x0D;  //en=1, rs=1
	data_t[3] = data_l|0x09;  //en=0, rs=1

	SendBytes(0x27, data_t, 4);
}

uint32_t SendBytes(uint8_t address, uint8_t* buffer, uint32_t length) {

    if (i2c0Busy)
        return 0;

    // Setear a ocupado
    i2c0Busy = 1;

    // setup pointers
    slaveAddress = address << 1;
    buf = buffer;
    bufLen = length;
    numTransferido = 0;

    // Trigger start
    LPC_I2C0->I2CONSET = STA;

    while (i2c0Busy);

    return numTransferido;
}

void I2C0_IRQHandler(void) {
    uint8_t status = LPC_I2C0->I2STAT;

    // ignorar data not ACK si control es true
    if ((status == 0x30) && (ignore_data_nack))
            status = 0x28;

    switch(status) {

    // Int: condicion de start enviada
    // Do:  enviar SLA+R or SLA+W
    case 0x08:
        LPC_I2C0->I2DAT = slaveAddress;
        LPC_I2C0->I2CONCLR = STA | SI;
        break;

    // Int: SLA+W transmitido, ACK recibido
    // Do:  enviar primer byte de buffer
    case 0x18:
        if (numTransferido < bufLen) {
            LPC_I2C0->I2DAT = buf[0];
            LPC_I2C0->I2CONCLR = STO | STA | SI;
        }
        else {
            LPC_I2C0->I2CONCLR = STA | SI;
            LPC_I2C0->I2CONSET = STO;
        }
        break;

    // Int: SLA+W has transmitido, not ACK recibido
    // Do:  stop
    case 0x20:
        LPC_I2C0->I2CONCLR = STA | SI;
        LPC_I2C0->I2CONSET = STO;
        numTransferido = 0xFFFFFFFF;
        i2c0Busy = 0;
        break;

    // Int: data byte transmitido, ACK recibido
    // Do:  cargar proximo byte o parar
    case 0x28:
        numTransferido++;
        if (numTransferido < bufLen) {
            LPC_I2C0->I2DAT = buf[numTransferido];
            LPC_I2C0->I2CONCLR = STO | STA | SI;
        }
        else {
            LPC_I2C0->I2CONCLR = STA | SI;
            LPC_I2C0->I2CONSET = STO;
            i2c0Busy = 0;
        }
        break;

    // Int: data byte enviado, not ACK recibido
    // Do:  stop
    case 0x30:
        LPC_I2C0->I2CONCLR = STA | SI;
        LPC_I2C0->I2CONSET = STO;
        i2c0Busy = 0;
        break;

    // Int: Perdida SLA+R/W o de data bytes
    // Do:  release bus
    case 0x38:
        LPC_I2C0->I2CONCLR = STO | STA | SI;
        i2c0Busy = 0;
        break;

    default:
        //Error jeje
        break;

    }
}
