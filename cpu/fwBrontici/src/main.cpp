#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <iostream>

#include "quadrature_encoder.hpp"
#include "buzzer.hpp"

#include "credentials.hpp"
#include "keys.hpp"
#include "driver/pcnt.h"

#define USE_AP

static const int MOTOR_PIN = 33;
static const int FUEL_PIN = 34;

int fuelPulses = 0;
static const int PULSE_TRIGGER = 2;

LiquidCrystal_I2C lcd ( 0x27, 20, 4 );
QuadratureEncoder encoder( GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14 );
Buzzer buzzer;
bool on = true;
bool overrideFuel = false;

static const char hexa[] = "0123456789ABCDEF";
static const char abc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

bool goodCode( const String& s ) {
    for ( int i = 0; i != 10; i++ ) {
        if ( keys[i] == s )
            return true;
    }
    return false;
}

struct Tones {
    void step() {
        if ( _busy )
            return;
        _busy = true;
        buzzer.beep( DAC_CHANNEL_2, 1000, 0b00 );
        delay( 50 );
        buzzer.stop( DAC_CHANNEL_2 );
        _busy = false;
    }

    void next() {
        if ( _busy )
            return;
        _busy = true;
        buzzer.beep( DAC_CHANNEL_2, 500, 0b00 );
        delay( 100 );
        buzzer.stop( DAC_CHANNEL_2 );
        _busy = false;
    }
    bool _busy;
};
Tones tones;

void fuckup() {
    for ( int i = 0; i != 3; i++ ) {
        for ( int i = 440; i < 4000; i *= 2 ) {
            buzzer.beep( DAC_CHANNEL_2, i, 0 );
            delay( 60 );
        }
        buzzer.stop( DAC_CHANNEL_2 );
    }
}

void famfare() {
    buzzer.beep( DAC_CHANNEL_2, 220, 0 );
    delay( 250 );
    buzzer.beep( DAC_CHANNEL_2, 440, 0 );
    delay( 250 );
    buzzer.beep( DAC_CHANNEL_2, 880, 0 );
    delay( 250 );
    buzzer.beep( DAC_CHANNEL_2, 440, 0 );
    delay( 250 );
    buzzer.beep( DAC_CHANNEL_2, 220, 0 );
    buzzer.stop( DAC_CHANNEL_2 );
};

void updateLcd( String code ) {
    lcd.setCursor( 0, 0 );
    lcd.clear();
    lcd.print( code.c_str() );
    lcd.write( hexa[ abs( encoder.bigSteps() ) % 16 ] );
    lcd.setCursor( code.length(), 0 );
}

struct Input {
    Input() : _direction( 1 ), _lastEncVal( encoder.bigSteps() )
    {}

    bool update() {
        int steps = encoder.bigSteps();
        if ( ( _direction > 0 && steps < _lastEncVal ) ||
             ( _direction < 0 && steps > _lastEncVal ) )
        {
            nextPosition();
            return true;
        }
        else if ( _direction < 0 && steps > _lastEncVal ) {
            nextPosition();

        }
        int l = _lastEncVal;
        _lastEncVal = steps;
        if ( l != steps ) {
            tones.step();
        }
        return l != steps;
    }

    void nextPosition() {
        char buff[ 2 ];
        buff[ 1 ] = 0;
        buff[ 0 ] = hexa[ abs( encoder.bigSteps() + _direction ) % 16 ];
        _code = _code + buff;
        _lastEncVal = 0;
        encoder.reset();
        _direction *= 1;
        if ( _code.length() == 16 && goodCode( _code ) ) {
            famfare();
            digitalWrite( MOTOR_PIN, LOW );
        }
        if ( _code.length() == 16 && !goodCode( _code ) ) {
            fuckup();
            _code = "";
            return;
        }
        tones.next();
    }

    int _direction;
    int _lastEncVal;
    String _code;
};
Input input;

void sendInfo( WiFiClient& client ) {
    static int counter = 0;
    client.print( "Frame: " );
    client.println( counter++ );
    client.println( " Controls \n"
                    "===========" );
    client.println( "  a: alert" );
    client.println( "  b: turn OFF" );
    client.println( "  c: turn ON" );
    client.println( "  d: reset" );
    client.println( "  n: disable fuel override" );
    client.println( "  f: famfare" );
    client.println( "  x: remove last char" );
    client.println( "  o: turn motor ON" );
    client.println( "  p: turn motor OFF" );

    client.println( " Status \n"
                    "=========" );
    client.print( "Fuel: " );
    client.println( fuelPulses );

    client.print( "Input: " );
    client.println( input._code );

    client.println( "-------------------------" );
}

void doAlert() {
    for ( int i = 0; i != 5; i++ ) {
        lcd.noBacklight();
        delay( 100 );
        lcd.backlight();
        delay( 100 );
        lcd.noBacklight();
        delay( 50 );
        lcd.backlight();
    }
    fuckup();
}

void turnOff() {
    digitalWrite( MOTOR_PIN, HIGH );
    lcd.noBlink();
    lcd.clear();
    lcd.noBacklight();
    input._code = "";
    on = false;
    buzzer.stop( DAC_CHANNEL_2 );
}

void turnOn() {
    digitalWrite( MOTOR_PIN, HIGH );
    lcd.backlight();
    lcd.blink();
    on = true;
}

void checkFuel() {
    static unsigned long long warn = 0;
    static unsigned long long shutdown = 0 ;
    if ( fuelPulses >= PULSE_TRIGGER ) {
        turnOn();
        warn = millis() + 2000;
        shutdown = millis() + 6000;
        return;
    }
    if ( shutdown < millis() ) {
        turnOff();
        return;
    }
    else if ( warn < millis() ) {
        warn = millis() + 1500;
        fuckup();
    }
}

void reset() {
    digitalWrite( MOTOR_PIN, HIGH );
    input._code = "";
}

void setupWifi( bool useDisplay = false ) {
#ifdef USE_AP
    WiFi.softAP( AP_SSID, AP_PASS );
    Serial.print( "IP: " );
    Serial.println( WiFi.softAPIP() );
    if ( useDisplay ) {
        lcd.setCursor( 0, 0 );
        lcd.print(WiFi.softAPIP().toString());
        delay( 4000 );
    }
#elif defined USE_STA
    WiFi.begin( C_SSID, C_PASS );
    int counter = 0;
    while( WiFi.status() != WL_CONNECTED ) {
        counter++;
        if ( counter == 15 ) {
            counter = 0;
            Serial.println("Trying again");
            WiFi.begin( C_SSID, C_PASS );
        }
        Serial.print( "Connecting... " );
        Serial.println( WiFi.status() );
        if ( useDisplay ) {
            lcd.setCursor( 0, 0 );
            lcd.print( "Connecting... " );
            lcd.println( WiFi.status() );
        }
        delay( 1000 );
    }
    Serial.print( "IP: " );
    Serial.println( WiFi.localIP() );
    if ( useDisplay ) {
        lcd.setCursor( 0, 0 );
        lcd.print( WiFi.localIP().toString() );
        delay( 4000 );
    }
#else
    #error "No mode defined"
#endif
}

void handleClient( WiFiClient& client ) {
    sendInfo( client );
    auto nextInfo = millis() + 1000;
    while( client.connected() ) {
        delay(100);
        if ( client.available() ) {
            int c = client.read();
            Serial.print( "Client command: ");
            Serial.write( c );
            Serial.println( "\n" );
            switch( c ) {
            case 'a':
            case 'A':
                doAlert();
                break;
            case 'b':
            case 'B':
                turnOff();
                overrideFuel = true;
                break;
            case 'c':
            case 'C':
                turnOn();
                overrideFuel = true;
                break;
            case 'n':
            case 'N':
                overrideFuel = false;
                break;
            case 'd':
            case 'D':
                reset();
                break;
            case 'f':
            case 'F':
                famfare();
                break;
            case 'x':
            case 'X':
                input._code.remove( input._code.length() - 1 );
                updateLcd( input._code );
                break;
            case 'o':
            case 'O':
                digitalWrite( MOTOR_PIN, LOW );
                break;
            case 'p':
            case 'P':
                digitalWrite( MOTOR_PIN, HIGH );
                break;
            }
        }
        if ( millis() > nextInfo ) {
            sendInfo( client );
            nextInfo = millis() + 1000;
        }
    }
}

void remoteControl( void* ) {
    WiFiServer server( 4242 );
    server.begin();
    while( true ) {
        delay( 1000 );
        auto client = server.available();
        if ( client )
            handleClient( client );
        if ( WiFi.getMode() == WIFI_MODE_STA &&  WiFi.status() != WL_CONNECTED ) {
            Serial.println( "Connection lost, setting up WiFi" );
            setupWifi();
        }
    }
}

void fueldRoutine( void* ) {
    pinMode(FUEL_PIN, INPUT);
    pcnt_config_t cfg = { -1 };
    cfg.pulse_gpio_num = FUEL_PIN;
    cfg.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    cfg.channel = PCNT_CHANNEL_0;
    cfg.unit = PCNT_UNIT_0;
    cfg.pos_mode = PCNT_COUNT_INC;   // Count up on the positive edge
    cfg.neg_mode = PCNT_COUNT_DIS;  // Keep the counter value on the negative edge
    cfg.lctrl_mode = PCNT_MODE_KEEP;
    cfg.hctrl_mode = PCNT_MODE_KEEP;
    cfg.counter_h_lim = 10000;
    cfg.counter_l_lim = 0;
    pcnt_unit_config(&cfg);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_intr_enable(PCNT_UNIT_0);

    while ( true ) {
        delay(1000);
        int16_t val;
        pcnt_get_counter_value(PCNT_UNIT_0, &val);
        fuelPulses = val;
        std::cout << "Fuel: " << fuelPulses << "\n";
        pcnt_counter_clear(PCNT_UNIT_0);
    }
}

void setup() {
    Serial.begin( 115200 );
    gpio_install_isr_service( 0 );
    encoder.start();
    pinMode( MOTOR_PIN, OUTPUT );
    digitalWrite( MOTOR_PIN, HIGH );
    lcd.init();
    lcd.backlight();
    lcd.blink_on();

    updateLcd( "" );
    encoder.reset();

    setupWifi( true );

    xTaskCreate( remoteControl, "control", 4096, nullptr, 5, nullptr );
    xTaskCreate( fueldRoutine, "fueldRoutine", 4096, nullptr, 5, nullptr );
}

int nextCheck = millis() + 2000;
void loop() {
    if( !overrideFuel && nextCheck < millis() ) {
        checkFuel();
        nextCheck = millis() + 500;
    }
    if ( !on )
        return;
    if ( input.update() )
        updateLcd( input._code );
    delay( 100 );
}