#include <Arduino.h>

static const int threshold = 990;
static const int samplesCount = 20;

void setup() {
    Serial.begin(9600);
    pinMode( A0, INPUT_PULLUP );
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

    if ( count > samplesCount * 3 / 4 )
        Serial.print("Y");
    else
        Serial.print("N");

    Serial.print( " " );
    Serial.println( avg / samplesCount );
}