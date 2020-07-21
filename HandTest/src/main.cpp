#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


const char *SECRET1 = "slovo1";
const char *SECRET2 = "slovo2";
const char *SECRET3 = "slovo3";
const char *SECRET4 = "slovo4";
const char *SECRET5 = "slovo5";
const char *SECRET6 = "slovo6";
const char *SECRET7 = "slovo7";


static const int threshold = 990;
static const int samplesCount = 20;

#define OLED_RESET 4
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char *getMessage() {
    if ( digitalRead(3) == 0 )
        return SECRET1;
    if ( digitalRead(4) == 0 )
        return SECRET2;
    if ( digitalRead(5) == 0 )
        return SECRET3;
    if ( digitalRead(6) == 0 )
        return SECRET4;
    if ( digitalRead(7) == 0 )
        return SECRET5;
    if ( digitalRead(8) == 0 )
        return SECRET6;
    if ( digitalRead(9) == 0 )
        return SECRET7;
    return "Unkwnow secret";
}

void setup() {
    Serial.begin(9600);
    pinMode( A0, INPUT_PULLUP );

    digitalWrite( 13, 0 );
    pinMode( 13, OUTPUT );
    digitalWrite( 13, 0 );

    display.begin( SSD1306_SWITCHCAPVCC, 0x3C );
    display.clearDisplay();
    display.display();

    pinMode(2, OUTPUT);
    digitalWrite(2, 0);

    for( int i = 3; i <= 9; i++ )
        pinMode(i, INPUT_PULLUP);
}

void loop() {
    int count = 0;
    int32_t avg = 0;
    for ( int i = 0; i != samplesCount; i++ ) {
        int val = analogRead( A0 );
        avg += val;
        if ( val < threshold )
            count++;

        delay(5);
    }

    if ( count > samplesCount * 3 / 4 ) {
        // Když je spojeno
        Serial.print("Y");
        display.setTextWrap( true );
        display.setTextSize( 1 );
        display.setTextColor( WHITE );
        display.setCursor( 0, 0 );
        display.println(getMessage());
    }
    else {
        // Když je rozpojeno
        Serial.print("N");
        display.clearDisplay();
    }
    display.display();

    Serial.print( " " );
    Serial.println( avg / samplesCount );
}