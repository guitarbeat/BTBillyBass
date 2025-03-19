// arduino-lint-disable
/*
 * Big Mouth Billy Bass - Audio Reactive Control
 * ============================================
 * Controls a Big Mouth Billy Bass animatronic fish using an Arduino and MX1508 motor drivers.
 * Features mouth and body/tail movement that reacts to audio input.
 * 
 * Hardware:
 * - Arduino board (tested on Uno/Nano)
 * - 2x MX1508 motor drivers
 * - Big Mouth Billy Bass toy
 * - Audio input connected to A0
 * 
 * Pins:
 * - A0: Audio input
 * - D3/D5: Mouth motor
 * - D6/D9: Body/tail motor
 * 
 * Commands:
 * o: Open mouth    c: Close mouth
 * f: Flap tail     b: Body forward
 * s: Singing mode  r: Reset position
 * +/-: Speed up/down
 * a: Toggle audio reactivity mode
 * m: Toggle manual/auto mode
 * d: Toggle debug mode
 * h: Help menu
 */

// Include all module headers
#include "src/core/Config.h"
#include "src/core/BillyBass.h"
#include "src/core/StateMachine.h"
#include "src/utils/Debug.h"

// Define global state structs
FishState fishState = {
    0,      // state
    true,   // audioReactivityEnabled
    true,   // manualMode - Changed to start in manual mode
    false,  // talking
    0       // soundVolume
};

TimingVars timing = {
    0,      // current
    0,      // mouthAction
    0,      // bodyAction
    0       // lastAction
};

// Define calibration settings
MovementCalibration calibration;  // Will use default values from struct definition

bool debugMode = false;

// Initialize the calibration settings in setup()
void initializeCalibration() {
    calibration.mouthOpenTime = 400;
    calibration.mouthCloseTime = 400;
    calibration.bodyForwardTime = 800;    // Increased from 600 to 800ms
    calibration.bodyBackTime = 800;       // Increased from 600 to 800ms
    calibration.mouthSpeed = 100;
    calibration.bodySpeed = 100;
}

// Simple command processing
void processCommand(char cmd) {
    static int value = 0;  // For processing numeric input
    
    switch (cmd) {
        case 'o':
            // Open mouth
            billy.openMouth();
            Serial.println(F("🐟 Opening mouth..."));
            break;
        case 'c':
            // Close mouth
            billy.closeMouth();
            Serial.println(F("🐟 Closing mouth..."));
            break;
        case 'f':
            // Flap tail
            billy.flapTail();
            Serial.println(F("🐟 Flapping tail... *splash*"));
            break;
        case 'b':
            // Body forward
            billy.bodyForward();
            Serial.println(F("🐟 Moving body forward..."));
            break;
        case 'r':
            // Reset position
            billy.resetMotorsToHome();
            Serial.println(F("🐟 Resetting to home position..."));
            break;

        // Calibration commands
        case 't': // Set mouth timing (open,close)
            Serial.println(F("\nMouth Timing Setup:"));
            Serial.println(F("Enter open time (ms): "));
            while (!Serial.available()) {}
            value = Serial.parseInt();
            calibration.mouthOpenTime = constrain(value, 0, MAX_MOVEMENT_TIME);
            
            Serial.println(F("Enter close time (ms): "));
            while (!Serial.available()) {}
            value = Serial.parseInt();
            calibration.mouthCloseTime = constrain(value, 0, MAX_MOVEMENT_TIME);
            
            Serial.print(F("Mouth timing set to: "));
            Serial.print(calibration.mouthOpenTime);
            Serial.print(F(","));
            Serial.println(calibration.mouthCloseTime);
            break;

        case 'y': // Set body timing (forward,back)
            Serial.println(F("\nBody Timing Setup:"));
            Serial.println(F("Enter forward time (ms): "));
            while (!Serial.available()) {}
            value = Serial.parseInt();
            calibration.bodyForwardTime = constrain(value, 0, MAX_MOVEMENT_TIME);
            
            Serial.println(F("Enter back time (ms): "));
            while (!Serial.available()) {}
            value = Serial.parseInt();
            calibration.bodyBackTime = constrain(value, 0, MAX_MOVEMENT_TIME);
            
            Serial.print(F("Body timing set to: "));
            Serial.print(calibration.bodyForwardTime);
            Serial.print(F(","));
            Serial.println(calibration.bodyBackTime);
            break;

        case 'm': // Set mouth speed
            Serial.println(F("\nEnter mouth speed (0-180): "));
            while (!Serial.available()) {}
            value = Serial.parseInt();
            calibration.mouthSpeed = constrain(value, 0, MAX_SPEED);
            Serial.print(F("Mouth speed set to: "));
            Serial.println(calibration.mouthSpeed);
            break;

        case 'n': // Set body speed
            Serial.println(F("\nEnter body speed (0-180): "));
            while (!Serial.available()) {}
            value = Serial.parseInt();
            calibration.bodySpeed = constrain(value, 0, MAX_SPEED);
            Serial.print(F("Body speed set to: "));
            Serial.println(calibration.bodySpeed);
            break;

        case 'p': // Print current settings
            Serial.println(F("\n=== Current Settings ==="));
            Serial.println(F("Mouth Timing (open,close):"));
            Serial.print(calibration.mouthOpenTime);
            Serial.print(F(","));
            Serial.println(calibration.mouthCloseTime);
            
            Serial.println(F("Body Timing (forward,back):"));
            Serial.print(calibration.bodyForwardTime);
            Serial.print(F(","));
            Serial.println(calibration.bodyBackTime);
            
            Serial.println(F("Speeds (mouth,body):"));
            Serial.print(calibration.mouthSpeed);
            Serial.print(F(","));
            Serial.println(calibration.bodySpeed);
            break;

        case 's':
            // Singing motion
            billy.singingMotion();
            Serial.println(F("Singing motion"));
            break;
        case '+':
            // Speed up
            billy.setMotorSpeed(min(255, billy.getMotorSpeed() + 5));
            Serial.print(F("Speed: "));
            Serial.println(billy.getMotorSpeed());
            break;
        case '-':
            // Speed down
            billy.setMotorSpeed(max(0, billy.getMotorSpeed() - 5));
            Serial.print(F("Speed: "));
            Serial.println(billy.getMotorSpeed());
            break;
        case 'a':
            // Toggle audio reactivity
            fishState.audioReactivityEnabled = !fishState.audioReactivityEnabled;
            Serial.print(F("🎤 Audio reactivity: "));
            Serial.println(fishState.audioReactivityEnabled ? F("ON - Billy will sing along!") : F("OFF - Billy is taking a break"));
            break;
        case 'l':
            // Toggle manual mode
            fishState.manualMode = !fishState.manualMode;
            Serial.print(F("🎮 Manual control: "));
            Serial.println(fishState.manualMode ? F("ON - You're in charge!") : F("OFF - Billy's on autopilot"));
            break;
        case 'd':
            // Toggle debug mode
            debugMode = !debugMode;
            Serial.print(F("🔧 Debug mode: "));
            Serial.println(debugMode ? F("ON - Showing technical details") : F("OFF - Keeping it simple"));
            break;
        case 'h':
        case '?':
            // Print menu
            printMenu();
            break;
        default:
            // Invalid command
            break;
    }
}

void printMenu() {
    Serial.println(F("\n🐟 === BILLY BASS CONTROL CENTER === 🐟"));
    Serial.println(F("\n📋 MOVEMENT COMMANDS:"));
    Serial.println(F("o: Open mouth    c: Close mouth"));
    Serial.println(F("f: Flap tail     b: Body forward"));
    Serial.println(F("r: Reset position"));
    
    Serial.println(F("\n⚙️ CALIBRATION:"));
    Serial.println(F("t: Set mouth timing (open,close)"));
    Serial.println(F("y: Set body timing (forward,back)"));
    Serial.println(F("m: Set mouth speed"));
    Serial.println(F("n: Set body speed"));
    Serial.println(F("p: Print current settings"));
    
    Serial.println(F("\n🔄 MODE CONTROLS:"));
    Serial.println(F("a: Audio react   d: Debug info"));
    Serial.println(F("l: Manual/auto   h: Show menu"));
    
    // Print current settings
    if (debugMode) {
        Serial.println(F("\n📊 CURRENT SETTINGS:"));
        Serial.print(F("Mouth timing (open,close): "));
        Serial.print(calibration.mouthOpenTime);
        Serial.print(F("ms, "));
        Serial.print(calibration.mouthCloseTime);
        Serial.println(F("ms"));
        
        Serial.print(F("Body timing (forward,back): "));
        Serial.print(calibration.bodyForwardTime);
        Serial.print(F("ms, "));
        Serial.print(calibration.bodyBackTime);
        Serial.println(F("ms"));
        
        Serial.print(F("Motor speeds (mouth,body): "));
        Serial.print(calibration.mouthSpeed);
        Serial.print(F(", "));
        Serial.println(calibration.bodySpeed);
    }
}

void setup() {
    // Initialize serial communication
    Serial.begin(9600);
    
    // Configure pins
    pinMode(SOUND_PIN, INPUT);
    
    // Initialize Billy Bass
    billy.begin();
    
    // Initialize calibration settings
    initializeCalibration();
    
    // Show welcome message and menu
    Serial.println(F("\n🎣 === WELCOME TO BILLY BASS === 🎣"));
    Serial.println(F("Your singing fish friend is ready to perform!"));
    Serial.println(F("Starting in manual mode - You're in control!"));
    Serial.println(F("Type 'h' for the command menu"));
    printMenu();
}

void loop() {
    // Update current time
    timing.current = millis();
    
    // Process serial commands if available
    if (Serial.available() > 0) {
        char input = Serial.read();
        processCommand(input);
    }
    
    // Run audio reactive mode if enabled and not in manual mode
    if (fishState.audioReactivityEnabled && !fishState.manualMode) {
        updateSoundInput();
        stateMachineBillyBass();
    }
} 