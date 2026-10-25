
#include <stdint.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"
#include "stdio.h"

#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

#define MAXRETRIES              5           // number of receive attempts before giving up
void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};																		// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
//    I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps clock
        
}

//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port G
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTG_PCTL_R = (GPIO_PORTG_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}

void PortM_Init(void){
	//Use PortM pins (PM0-PM3) for output
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;				// activate clock for Port M
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R11) == 0){};	// allow time for clock to stabilize
	GPIO_PORTM_DIR_R |= 0x0F;        								// configure Port M pins (PM0-PM3) as output
  GPIO_PORTM_AFSEL_R &= ~0x0F;     								// disable alt funct on Port M pins (PM0-PM3)
  GPIO_PORTM_DEN_R |= 0x0F;        								// enable digital I/O on Port M pins (PM0-PM3)
																									// configure Port M as GPIO
  GPIO_PORTM_AMSEL_R &= ~0x0F;     								// disable analog functionality on Port M	pins (PM0-PM3)	
	return;
}

void PortJ_Init(void){
	//Use PortJ pins (PM0-PM3) for output
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8;				// activate clock for Port J
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R8) == 0){};	// allow time for clock to stabilize
	GPIO_PORTJ_DIR_R &= ~0x03;        								// configure Port J pins (PJ0-PJ3) as output
  GPIO_PORTJ_AFSEL_R &= ~0x03;     								// disable alt funct on Port J pins (PJ0-PJ1)
  GPIO_PORTJ_DEN_R |= 0x03;        								// enable digital I/O on Port J pins (PJ0-PJ1)
																									// configure Port J as GPIO
  GPIO_PORTJ_AMSEL_R &= ~0x03;     								// disable analog functionality on Port J	pins (PJ0-PJ1)
	
	GPIO_PORTJ_PUR_R |= 0x03;
	GPIO_PORTJ_PCTL_R &= ~0x000000FF;
	return;
}
void PortN_Init(void){
	//Use PortM pins (PN0-PN1) for output
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;				// activate clock for Port n
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};	// allow time for clock to stabilize
	GPIO_PORTN_DIR_R |= 0x03;        								// configure Port n pins (Pn0-Pn1) as output
  GPIO_PORTN_AFSEL_R &= ~0x03;     								// disable alt funct on Port n pins 
  GPIO_PORTN_DEN_R |= 0x03;        								// enable digital I/O on Port n pins
																									// configure Port n as GPIO
  GPIO_PORTN_AMSEL_R &= ~0x03;     								// disable analog functionality on Port n	pins	
	return;
}



//XSHUT     This pin is an active-low shutdown input; 
//					the board pulls it up to VDD to enable the sensor by default. 
//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
    
}





//***********					MAIN Function				*****************************************************************
//*********************************************************************************************************
//*********************************************************************************************************
uint16_t	dev = 0x29;			//address of the ToF sensor as an I2C slave peripheral
int status=0;

int main(void) {
 // initialize everything 
  uint8_t byteData, sensorState=0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} , i=0;
  uint16_t wordData;
  uint16_t Distance;
  uint16_t SignalRate;
  uint16_t AmbientRate;
  uint16_t SpadNum; 
  uint8_t RangeStatus;
  uint8_t dataReady = 0;
	int dists[512];
	int count = 0;

	char I;
	char TxChar;
	int input = 0;
	int data_array[3];
	

	//initialize
	PLL_Init();	
	SysTick_Init();
	onboardLEDs_Init();
	I2C_Init();
	UART_Init();
	PortM_Init();																		
	PortJ_Init();
	PortN_Init();
	

	
	// 1 Wait for device booted
	while(sensorState==0){
		status = VL53L1X_BootState(dev, &sensorState);
		SysTick_Wait10ms(10);
  }
	UART_printf("ToF Chip Booted!\r\n Please Wait...\r\n");
	
	status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
	
  /* 2 Initialize the sensor with the default setting  */
  status = VL53L1X_SensorInit(dev);
	Status_Check("SensorInit", status);

while(1){
		if(GPIO_PORTJ_DATA_R == 0x01){
	FlashLED2(1);
	status = VL53L1X_StartRanging(dev) ;   // This function has to be called to enable the ranging
	while(count < 512){ //get 152 scans
  
	uint32_t delay = 1;															
	
	for(int i=0; i< 512 ; i++){	//Steps for rotation
		GPIO_PORTM_DATA_R = 0b1100; 
		SysTick_Wait10ms(delay);											
		GPIO_PORTM_DATA_R = 0b0110 ;													// C
		SysTick_Wait10ms(delay);
		GPIO_PORTM_DATA_R = 0b0011;													
		SysTick_Wait10ms(delay);
		GPIO_PORTM_DATA_R = 0b1001;													
		SysTick_Wait10ms(delay);
		
		if (i % 8 ==0){ //check for rotation to take a measurement
		while (dataReady == 0){
		  status = VL53L1X_CheckForDataReady(dev, &dataReady);
      FlashLED1(1); //flash LED when taking a measurement
      VL53L1_WaitMs(dev, 5);
	  }
		//measure and increase count indicate one measurement taken
		dataReady = 0;
		status = VL53L1X_GetDistance(dev, &Distance);
    status = VL53L1X_ClearInterrupt(dev);
		dists[count] = Distance;
		count++;

		}
	}
	for(int i=0; i< 512 ; i++){//go back so wires dont get tangled between scans												
		GPIO_PORTM_DATA_R = 0b1001; 
		SysTick_Wait10ms(delay);											
		GPIO_PORTM_DATA_R = 0b0011 ;													
		SysTick_Wait10ms(delay);
		GPIO_PORTM_DATA_R = 0b0110;													
		SysTick_Wait10ms(delay);
		GPIO_PORTM_DATA_R = 0b1100;													
		SysTick_Wait10ms(delay);
	}
 }
		VL53L1X_StopRanging(dev);//stop ranging
}
	if(GPIO_PORTJ_DATA_R == 0x02){
		FlashLED3(1);
	while(1){FlashLED3(10);}
	while(1){//check for beginning of communication
		input = UART_InChar();
		if (input == 's')
				break;
	}
	for(int j = 0; j <512; j++){//send all data over UART
	FlashLED3(1);//flash when sending
	if(dists[j] < 4000){//4m is max filters out garbage values
  sprintf(printf_buffer, "%u\r\n", dists[j]);
	}
	else{
	sprintf(printf_buffer, "%u\r\n", 4000);
		                                                                                                                                                                                                                  
	}
  UART_printf(printf_buffer);
}	
	UART_printf("Scan complete\r\n");
  GPIO_PORTM_DATA_R = 0b0000;
}}}
