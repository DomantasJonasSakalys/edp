#include "mbed.h"

#define max7219_reg_noop         0x00
#define max7219_reg_digit0       0x01
#define max7219_reg_digit1       0x02
#define max7219_reg_digit2       0x03
#define max7219_reg_digit3       0x04
#define max7219_reg_digit4       0x05
#define max7219_reg_digit5       0x06
#define max7219_reg_digit6       0x07
#define max7219_reg_digit7       0x08
#define max7219_reg_decodeMode   0x09
#define max7219_reg_intensity    0x0a
#define max7219_reg_scanLimit    0x0b
#define max7219_reg_shutdown     0x0c
#define max7219_reg_displayTest  0x0f

#define LOW 0
#define HIGH 1

AnalogIn Ain(PTB0);

SPI max72_spi(PTD2, NC, PTD1);
DigitalOut load(PTD3);
DigitalOut led(LED1);

const unsigned int SAMPLES = 400, TIMEOUT = 25;
unsigned int pointer, noleds, current, min, max, avg, beats = 0, bac = 0, beatsa[4], store[400], screen[8];
bool beatsamples = 0;
char lines[9] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF};
char upload[8];
char pattern[8] = { 0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff};
char numbers[10][4] = {{0xff, 0x81, 0x81, 0xff}, {0x00, 0x00, 0xff, 0x00}, {0xf9, 0x89, 0x89, 0x8f},//0, 1, 2 
                        {0x89, 0x89, 0x89, 0xff}, {0x0f, 0x08, 0x08, 0xff}, {0x8f, 0x89, 0x89, 0xf9},//3, 4, 5
                        {0xff, 0x89, 0x89, 0xf9}, {0x01, 0x01, 0x01, 0xff}, {0xff, 0x89, 0x89, 0xff},//6, 7, 8
                        {0x8f, 0x89, 0x89, 0xff}};//9

//MAX code

//write whole matrix
void write_to_max( int reg, int col)
{
    load = LOW; // begin
    max72_spi.write(reg); // specify register
    max72_spi.write(col); // put data
    load = HIGH; // make sure data is loaded (on rising edge of LOAD/CS)
}

//writes 8 bytes to the display  
void pattern_to_display(char *testdata){
    int cdata; 
    for(int idx = 0; idx <= 7; idx++) {
        cdata = testdata[idx]; 
        write_to_max(idx+1,cdata);
    }
}

//empty registers, turn all LEDs off
void clear(){
     for (int e=1; e<=8; e++) {
        write_to_max(e,0);
    }
}

//initiation of the max 7219
void setup_dot_matrix()
{
    max72_spi.format(8, 0); // SPI setup: 8 bits, mode 0
    max72_spi.frequency(100000); //down to 100khx easier to scope ;-)

    write_to_max(max7219_reg_scanLimit, 0x07);
    write_to_max(max7219_reg_decodeMode, 0x00); //using an led matrix (not digits)
    write_to_max(max7219_reg_shutdown, 0x01); //not in shutdown mode
    write_to_max(max7219_reg_displayTest, 0x00);//no display test
    write_to_max(max7219_reg_intensity,  0x08); //led brightness 
    clear(); 
}


//Added functions

void numbers_to_display(char data1[4], char data2[4], bool hundred){
    int cdata; 
    for(int idx = 0; idx <= 3; idx++) {
        cdata = data1[idx];
        write_to_max(idx+1,cdata);
    }
    for(int idx = 0; idx <= 3; idx++) {
        cdata = data2[idx];
        write_to_max(idx+5,cdata);
    }
    led = !hundred;
}

//display digits
void view_bpm(int bpm){
    int digit1 = bpm % 10;
    int digit2 = bpm / 10;
    int digit3 = bpm / 100;
    digit2 = digit2 % 10;
    digit3 = digit3 % 10;
    bool h = 0;
    if(digit3 != 0)h = 1;
    numbers_to_display(numbers[digit2], numbers[digit1], h);
    }

//MAIN code

int main()
{
    //Init
    setup_dot_matrix();
    for(int a=0;a<8;a++){
        upload[a] = 0;
    }
    led = 0;
    //Main loop
    for(int i = 0; i <=SAMPLES; i++){
        //read bpm 
        if(i == SAMPLES){
            i = 0;
            bool pulse = 0;
            for(int a = 0; a < SAMPLES; a++){
                if(!pulse){ 
                    if (8 * (store[a] - min) / (max - min) >= 3){
                        pulse = 1;
                        beats++;
                    }
                }
                else if(8 * (store[a] - min) / (max - min) <= 1) pulse = 0;
            }
            view_bpm(60000*beats/SAMPLES/TIMEOUT);
            beats = 0;
            wait(5);
        }
        
        //get data
        store[i] = Ain.read_u16();
        
        //Min, max, avg
        min = store[0];
        max = store[0];
        avg = store[0];
        for(int a = 1; a < SAMPLES; a++){
            if(store[a] < min)min = store[a];
            if(store[a] > max)max = store[a];
            avg += store[a];
        }
        avg = avg / SAMPLES;
        
        //Print a line
        if(min/10 == max/10){
            pattern_to_display(pattern);
            i = 0;
            }
        else {
            for(int a=1;a<8;a++){
                upload[a-1] = upload[a];
            }
            int value = 8 * (store[i] - min) / (max - min);
            upload[7] =  lines[value];
            pattern_to_display(upload);
        }
        
        //timeout
        wait(TIMEOUT / 1000.0);          
    }

}