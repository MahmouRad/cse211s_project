#include "mbed.h"
using namespace chrono;


// Hardware Pin Definitions

// Shift register control pins
DigitalOut latchPin(D4);  // Controls latch line of shift register
DigitalOut clockPin(D7);  // Controls clock line of shift register
DigitalOut dataPin(D8);   // Sends serial data to shift register

// User input buttons
DigitalIn resetButton(A1);        // Button to reset timer
DigitalIn voltageDisplayButton(A3); // Button to display voltage

// Analog input (potentiometer)
AnalogIn potentiometer(A0);  // Reads analog voltage from potentiometer


// Constants: 7-Segment Display Data


// Segment patterns for digits 0-9 (common anode, active-low)
const uint8_t segmentPatterns[10] = {
    static_cast<uint8_t>(~0x3F), // 0
    static_cast<uint8_t>(~0x06), // 1
    static_cast<uint8_t>(~0x5B), // 2
    static_cast<uint8_t>(~0x4F), // 3
    static_cast<uint8_t>(~0x66), // 4
    static_cast<uint8_t>(~0x6D), // 5
    static_cast<uint8_t>(~0x7D), // 6
    static_cast<uint8_t>(~0x07), // 7
    static_cast<uint8_t>(~0x7F), // 8
    static_cast<uint8_t>(~0x6F)  // 9
};

// Digit select positions for multiplexing (left to right)
const uint8_t digitSelect[4] = { 0x01, 0x02, 0x04, 0x08 };

// Global Variables

// Timekeeping variables
volatile int currentSeconds = 0;
volatile int currentMinutes = 0;

// Variables for tracking minimum and maximum voltage values
volatile float minimumVoltage = 3.3f;
volatile float maximumVoltage = 0.0f;

// Ticker for periodic time updates
Ticker secondTicker;

// Function Declarations
void incrementTime();
void shiftOutByte(uint8_t value);
void updateShiftRegister(uint8_t segments, uint8_t digit);
void displayFourDigitNumber(int number, bool showDecimal = false, int decimalPos = -1);


// Interrupt Service Routine: 1s Timer
void incrementTime() {
    currentSeconds++;
    if (currentSeconds >= 60) {
        currentSeconds = 0;
        currentMinutes = (currentMinutes + 1) % 100;
    }
}


// Shift Register Communication

// Shift out a byte (MSB first) to shift register
void shiftOutByte(uint8_t value) {
    for (int bitIndex = 7; bitIndex >= 0; --bitIndex) {
        dataPin = (value & (1 << bitIndex)) ? 1 : 0;
        clockPin = 1;
        clockPin = 0;
    }
}

// Write segments and digit position to the shift register
void updateShiftRegister(uint8_t segments, uint8_t digit) {
    latchPin = 0;
    shiftOutByte(segments); // First send segment data
    shiftOutByte(digit);    // Then send digit select
    latchPin = 1;
}

// 7-Segment Display Output

// Display a 4-digit number with optional decimal point
void displayFourDigitNumber(int number, bool showDecimal, int decimalPos) {
    int individualDigits[4] = {
        (number / 1000) % 10,
        (number / 100)  % 10,
        (number / 10)   % 10,
        number % 10
    };

    // Iterate through each digit position
    for (int digitIndex = 0; digitIndex < 4; ++digitIndex) {
        uint8_t segments = segmentPatterns[individualDigits[digitIndex]];

        // Enable decimal point if applicable
        if (showDecimal && digitIndex == decimalPos)
            segments &= ~0x80;  // Clear DP bit (active low)

        updateShiftRegister(segments, digitSelect[digitIndex]);
        ThisThread::sleep_for(2ms);
    }
}


int main() {
    // Initialize button inputs with pull-up resistors
    resetButton.mode(PullUp);
    voltageDisplayButton.mode(PullUp);

    // Attach ticker interrupt to increment time every second
    secondTicker.attach(&incrementTime, 1s); // using chrono literal

    while (true) {
        // Reset time when reset button is pressed
        if (!resetButton) {
            currentSeconds = 0;
            currentMinutes = 0;
            ThisThread::sleep_for(200ms); // Debounce delay
        }

        // Read and convert potentiometer voltage (0V to 3.3V)
        float voltage = potentiometer.read() * 3.3f;

        // Update min and max voltage values
        if (voltage < minimumVoltage) minimumVoltage = voltage;
        if (voltage > maximumVoltage) maximumVoltage = voltage;

        // Display either voltage or time based on button press
        if (!voltageDisplayButton) {
            // Display scaled voltage value as X.XX
            int scaledVoltage = static_cast<int>(voltage * 100); // Example: 2.45V → 245
            displayFourDigitNumber(scaledVoltage, true, 1);      // Show decimal after first digit
        } else {
            // Display current time in MMSS format
            int timeValue = currentMinutes * 100 + currentSeconds;
            displayFourDigitNumber(timeValue);
        }
    }
}

