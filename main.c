#include <LPC17xx.h>
#include "lcd.h"
#include "uart.h"
#include "math.h"

#define BUZZER_PIN(X) (1 << X)
#define SWITCH_PIN1(X) (1 << X)
#define SWITCH_PIN2(Y) (1 << Y)

#define CAN2_BAUD_RATE 200000  // Set the desired baud rate for CAN communication as 100Kbps
							   // So nominal bit time = 10 us

#define CAN1_BAUD_RATE 200000  // Set the desired baud rate for CAN communication as 100Kbps
							   // So nominal bit time = 10 us

void CAN2_Init(void);
void CAN1_Init(void);

void CAN2_MessageSend(uint32_t msgId, uint64_t data, uint8_t length);				 //transmitter function	, can2 is transmitter
void CAN1_MessageReceive(uint32_t *msgId, uint64_t *data, uint8_t *length);


void timer1_delay_us(unsigned int us)
{
   	LPC_TIM3->MR0 = us;
	LPC_TIM3->TCR =(1<<0);
}

void carEntrySystem() {
    char pass[] = "abc";
    int s = 3;
    char z;
    int i, count;
    int f = 0;

    LPC_GPIO1->FIODIR |= BUZZER_PIN(27);
    LPC_GPIO1->FIOCLR = BUZZER_PIN(27);
    lcd_init();
    uart_config();

    while (f != 1) {
        i = 0;
        count = 0;

        lcd_cmd_write(0X0C);
        lcd_cmd_write(0X01);
        lcd_str_write("Enter password:");
        lcd_cmd_write(0XC0);

        while (i < s) {
            z = uart_rx();
            lcd_data_write(z);
            if (pass[i] == z) {
                count++;
            }
            i++;
        }
        delay(100);
        lcd_cmd_write(0X01);
        if (count == s) {
            lcd_str_write("Password Correct");
            lcd_cmd_write(0XC0);
            lcd_str_write("Car unlocked");
            f = 1;
        } else {
            lcd_str_write("Wrong Password!");
            lcd_cmd_write(0XC0);
            lcd_str_write("Car locked");
            LPC_GPIO1->FIOSET = BUZZER_PIN(27);
            delay(250);
            LPC_GPIO1->FIOCLR = BUZZER_PIN(27);
        }
        delay(100);
    }
}

void doorStatusDisplay() {
    int	flag=0;
    uint32_t tmsgId = 0x123;
    uint64_t tmessageData;
    uint8_t tlength = 3;

    uint32_t rmsgId;
    uint64_t rmessageData;
    uint8_t rlength;

    char id_data[20];

    lcd_init();

    CAN2_Init();
    CAN1_Init();

    LPC_GPIO2->FIODIR &= ~(SWITCH_PIN1(11));
    LPC_GPIO2->FIODIR &= ~(SWITCH_PIN2(12));

    while (flag<2) {

        if ((LPC_GPIO2->FIOPIN & SWITCH_PIN2(12)) != 0) {
            tmessageData = 0x00;
            CAN2_MessageSend(tmsgId, tmessageData, tlength);
            CAN1_MessageReceive(&rmsgId, &rmessageData, &rlength);

            sprintf(id_data, "%x", rmessageData);
            lcd_str_write(id_data);
            lcd_str_write(" door is closed");
			if(flag == 1){
				flag += 1;
			}
            delay(250);
            lcd_cmd_write(0x01);
        }

        if ((LPC_GPIO2->FIOPIN & SWITCH_PIN1(11)) != 0) {
            tmessageData = 0x01;
            CAN2_MessageSend(tmsgId, tmessageData, tlength);
            CAN1_MessageReceive(&rmsgId, &rmessageData, &rlength);

            sprintf(id_data, "%x", rmessageData);
            lcd_str_write(id_data);
            lcd_str_write(" door is open");
            if(flag == 0){
				flag += 1;
			}
			delay(250);
            lcd_cmd_write(0x01);
        }
    }
}
void timer3(void)
{
	LPC_SC->PCONP |= (1 << 23); 
	LPC_SC->PCLKSEL1 &= ~(1 << 14); 
	LPC_SC->PCLKSEL1 &= ~(1 << 15); 

	LPC_TIM3->CTCR = 0X00;		//timer mode //everything same for tim0,1,2,3.	                            
	                             //just change pointer name to LPC_TIM0,LPC_TIM1,LPC_TIM2,LPC_TIM3.
	LPC_TIM3->PR = 0X0;  
    LPC_TIM3->TCR = (1<<1);
	LPC_TIM3->MCR |= (1<<1)|(1<<0);

	//uint32_t i;
    //timer1_init(); 	
	
	LPC_GPIO1->FIODIR |= (1<<27);
	
	timer1_delay_us(5000000);
     //int cnt=0;                                   
	
	while(1){
		if(LPC_TIM3->IR & (1<<0))
	 { 
	   LPC_GPIO1->FIOSET |= (1<<27);
	   delay(500);
	   LPC_GPIO1->FIOCLR = (1<<27);
	   delay(500);
	   LPC_TIM3->IR = 1<<0;
	   break;
	 }
	}
	 
}

void distance() {
	int i, j, sp, sd = 7;
	float cd;
	char arr[16];

	LPC_GPIO2 -> FIODIR |= (1<<11);
	LPC_GPIO2 -> FIOCLR |= (1<<11);
	LPC_GPIO1 -> FIODIR |= (1<<27);

	lcd_init();
	lcd_cmd_write(0X01);
	lcd_cmd_write(0X0C);

	for(i = 0, j = 0, sp = 0; i < 10; i++, j++, sp += 10){
			cd = sqrt((i*i)+(j*j));
			if(i==7)
		 	{ 
		   		lcd_cmd_write(0x01); 
		   		lcd_str_write("stopage occured");
		   		
		   		timer3();
				lcd_cmd_write(0x01);
		 	}
			sprintf(arr,"Peri:%d Dist:%.2f", sd, cd);
			lcd_str_write(arr);
			lcd_cmd_write(0XC0);
			if(cd < sd){
				lcd_str_write("In");
			}
			else{
				lcd_str_write("Out");
			}
			lcd_cmd_write(0X14);
			lcd_cmd_write(0X14);
			sprintf(arr,"Speed:%d", sp);
			lcd_str_write(arr);
			if(sp>=60){
				LPC_GPIO1 -> FIOSET |= (1<<27);
				delay(sp*2);
				LPC_GPIO1 -> FIOCLR |= (1<<27);
			}
			delay(200);
			lcd_cmd_write(0X01);
	}
}

int main() {
    lcd_init();
	carEntrySystem();
	lcd_cmd_write(0X01);
	lcd_str_write("Open the door");
	delay(100);
    doorStatusDisplay();
	lcd_cmd_write(0X01);
	lcd_str_write("Journey Started");
	delay(100);
	distance();
	lcd_cmd_write(0X01);
	lcd_str_write("Journey Complete");
	lcd_cmd_write(0X01);
	lcd_str_write("Thank You");
	delay(5000);
	lcd_cmd_write(0X01);
	
	//timer3();
    return 0;
}
void CAN2_Init(void) 										   //can2 must be configured as transmitter
{
    // Set P2.7 as RD2 and P2.8 as TD2 CAN2 PINS
    LPC_PINCON->PINSEL4 |=  (1 << 14);//receiver 15,14 - 01
    LPC_PINCON->PINSEL4 &= ~(1 << 15);
    LPC_PINCON->PINSEL4 |=  (1 << 16);//transmitter 17,16 - 01
    LPC_PINCON->PINSEL4 &= ~(1 << 17);
    
    // Enable power to CAN2 controller
    LPC_SC->PCONP |= (1 << 14);
    
    // Set CAN2 in Reset mode
    LPC_CAN2->MOD = 1;
    
    // Configure the Baud Rate for CAN2
    LPC_CAN2->BTR = (1<<23)|(2<<20)|(0<<16)|(0X00); // CAN2 clock = PCLK/(0+1)= 1MHz/1=1000KHz  i.e Time Quanta = 1 us
                                                    // i.e total time quanta = 10us/1us = 10
                                                    // (1+TSEG1)/(1+TSEG1+TSEG2)= 80% => (1+TSEG1)/10 = 80% => TSEG1 = 7 and TSEG2 = 2
    
    // Set CAN2 in Normal mode
    LPC_CAN2->MOD = 0;
}

// Function to send a CAN message using CAN2 controller
void CAN2_MessageSend(uint32_t msgId, uint64_t data, uint8_t length) 
{
    // Wait until Transmit Buffer 1 (TBR1) is empty
    while((LPC_CAN2->GSR & (1 << 2)) == 0){ }	//we are checking if transmitter buffer register is empty to hold the data
    
    // Set the message ID
    LPC_CAN2->TID1 = msgId;
    
    // If extended message ID, set the Extended Identifier Flag
    if(msgId > 0x7FF)								  //07ff is the max number we can write using 11 bits.
    {
        LPC_CAN2->TFI1 |= (1U << 31);				   //create extended frame is id is greater than 11 bits , creating a 29 bits frame	for accomodate 12 bits
    }

    // Set data length code
    LPC_CAN2->TFI1 |= ((uint32_t)length << 16);			//craeting extra space pf 3 bits to accomodate extra bits above 11bits

    // If data length is greater than 4 bytes, set the first 4 bytes of data
    if(length > 4)						    //register capacity is 4 bytes, if data is greater than 4 than use another register TDB1
    {
        LPC_CAN2->TDB1 = data >> 32;		//extra bits will go in TDB1, remaining previous 4 bytes of data will be accomodated in TDA1.
    }
    
    // Set the next 4 bytes of data
    LPC_CAN2->TDA1 = (uint32_t)data;		//extra bits will go in TDB1, remaining previous 4 bytes of data will be accomodated in TDA1.
    
    // Enable queued transmission and select Tx Buffer1
    LPC_CAN2->CMR |= (1 << 0)|(1 << 5);		//both tda and tdb will give data to transmitter buffer
    													  
    // Wait for transmission completion
    while ((LPC_CAN2->GSR & (1 << 3)) == 0) { }	//we are checking if transmittion is successful. 
}

// Function to initialize CAN1 controller
void CAN1_Init(void) 
{
    // Set P0.0 as RD1 and P0.1 as TD1
    LPC_PINCON->PINSEL0 |= (1 << 0);
    LPC_PINCON->PINSEL0 &= ~(1 << 1);
    LPC_PINCON->PINSEL0 |= (1 << 2);
    LPC_PINCON->PINSEL0 &= ~(1 << 3);
    
    // Enable power to CAN1 controller
    LPC_SC->PCONP |= (1 << 13);
    
    // Set CAN1 in Reset mode
    LPC_CAN1->MOD = 1;
    
    // Configure the Baud Rate for CAN1
    LPC_CAN1->BTR =(1<<23)|(2<<20)|(0<<16)|(0X00); // CAN1 clock = PCLK/(0+1)= 1MHz/1=1000KHz  i.e Time Quanta = 1 us
                                                    // i.e total time quanta = 10us/1us = 10
                                                    // (1+TSEG1)/(1+TSEG1+TSEG2)= 80% => (1+TSEG1)/10 = 80% => TSEG1 = 7 and TSEG2 = 2
    
    // Set CAN1 in Normal mode
    LPC_CAN1->MOD = 0;
}

// Function to receive a CAN message using CAN1 controller
void CAN1_MessageReceive(uint32_t *msgId, uint64_t *data, uint8_t *length) 
{
    // Wait until Receive Buffer (RBR) is not empty
    while((LPC_CAN1->GSR & (1<<0))==1){ }  //we are checking if reciever buffer register is empty to hold the data.
    
    // Load the message ID to a variable
    *msgId = LPC_CAN1->RID;

    // Extract data length code from the frame status
    *length = (LPC_CAN1->RFS >> 16) & 0x0F ;
    
    // Load the first 4 bytes of received data
    *data = LPC_CAN1->RDA;
    
    // If data length is greater than 4 bytes, load the next 4 bytes of data
    if(*length > 4)
    {
        *data = *data | ((uint64_t)LPC_CAN1->RDB << 32);
    }

    // Release the buffer
    LPC_CAN1->CMR=(1<<2);
}