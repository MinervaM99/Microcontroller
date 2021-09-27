#define FOSC 16000000 // viteza clock
#define BAUD 9600  //viteza seriala de transmisie
#define MYUBRR FOSC/16/BAUD-1 

unsigned int numar_CAN;
float VoltageValue, tempValue;
unsigned char date_transmise;
int count, i;

int main()
{
  set_bcd7();
  set_timer1();
  set_PWM_t2();
  adc_init();
  USART_Init(MYUBRR);
  sei();
  while(1){}
}


////////////////////////////////////////////

ISR(TIMER1_OVF_vect)
{
  PORTB ^=1<<5;
  //pt LCD (litera M)
  PORTD ^= 0B10011000;
  PORTB ^= 0B0011; 
  
  int intTmp;
  VoltageValue= (float)numar_CAN*5/1023;
  tempValue =-40+100*(VoltageValue-0.1); 
  intTmp=(int)(tempValue*100);
  if(tempValue>35.5)
    PORTD |= 1<<PD6;
  if(tempValue<34.5)
    PORTD &= ~(1<<PD6);
  
  char buf[100];
  memset(buf, 0, sizeof(buf));
  sprintf(buf, "Temperatura este %d.%d *C \n", intTmp/100, intTmp%100);
  for (int i = 0; i < strlen(buf); i++)
     USART_Transmit(buf[i]);
     
  TCNT1=49910; 
}

void set_timer1()
{
  DDRB|=1<<5;
  //pentru LCD
  PORTD |= 0B10111100; 
  PORTB |= 0B0011;
  //stergem registrii de control
  TCCR1A=0;
  TCCR1B=0;
  TCCR1C=0;
  //configuram modul de lucru -- NORMAL
  //prescaler 1024
  TCCR1B|= 1<<CS10 | 1<<CS12;
  //intrerupere de ovf
  TIMSK1 |= 1<<TOIE1;
  TCNT1=49910; 
}

/////////////////////////////////////////////////////////
void set_PWM_t2()
{
  TCCR2A |= 1 << COM2A1; //OC0A Non-inverting mode
  TCCR2A |= (1 <<WGM20) | (1 <<WGM21); //Fast PVM
  TCCR2B |= (1 <<CS22); //Prescale 64
  TIMSK2 |= (1<<TOIE2); //intrerupere ovf
  OCR2A=0;
  DDRB|=1<<3;
}

ISR(TIMER2_OVF_vect)
{
  count++;
  if(count==4)
  {
    i++;
    if(i<255)
     OCR2A=i;
    else if(i>=255 && i<510)
     OCR2A=255;
    else if(i>=510 && i<765)
     OCR2A--;
    else if(i>=765 && i<=1020)
     OCR2A=0;
    else i=0;
    count=0;
  }
}
/////////////////////////////////////////////
void set_bcd7()
{
  DDRD |=(1<<2) | (1<<3) | (1<<4) | (1<<5)| (1<<7);
  DDRB |=(1<<1) | (1<<0);
}
///////////////////////////////////////////////////////

void USART_Transmit(unsigned char data)
{
  while(!(UCSR0A & (1<<UDRE0)));
  UDR0=data;
}
unsigned char USART_Receive(void)
{
  while(!(UCSR0A & (1<<RXC0))); //asteapta datele
  return UDR0; 
}
void USART_Init(unsigned int ubrr)
{
  DDRB |= 1<<PB2;
  UBRR0H = (unsigned char)(ubrr>>8); //baud set
  UBRR0L = (unsigned char)ubrr;
  UCSR0B = (1<<RXEN0)|(1<<TXEN0) | (1<<RXCIE0);
  UCSR0C = (1<<UCSZ01)|(1<<UCSZ00); //8 biti de date si 2 biti de stop
}
ISR(USART_RX_vect)  
{
  char var = USART_Receive();
    if(var=='A')
    {
      PORTB |= (1<<PB2);
    }
    else if(var=='S')
    {
      PORTB &= ~(1<<PB2);
    }
}
void adc_init()  
{
  ADCSRA |= ((1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)); 
  ADMUX |= (1<<REFS0);   // Setam ca referinta 5V.
  ADCSRA |= (1<<ADEN);  // Alimentarea perifericului.
  ADCSRA |= 1 << ADIE; // Pornim intreruperile ADC.
  ADCSRA |= (1<<ADSC);  //ADC start conversion
  DDRD |= 1 << PD6; 
}
uint16_t read_adc(uint8_t channel)
{
  ADMUX &= 0xF0;  //set input AO to A5
  ADMUX |= channel; //select chanel AO to A5
  ADCSRA |= (1<<ADSC);  //start conversion
  while(ADCSRA & (1<<ADSC));  //asteapta până când conversia adc este actualizată (bitul e 0)
  return ADC;  //returneaza tensiunea corespunzatoare
}

ISR(ADC_vect)
{ 
  numar_CAN = read_adc(0);
  ADCSRA |= 1<<ADSC; //start conversion
}
