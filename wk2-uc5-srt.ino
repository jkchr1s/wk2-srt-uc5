// big thanks to stoopalini!!!
//
//0x3A0 = Drive mode button presses from the uConnect 5 system. One time event, not continual broadcast
//0x2FA = Byte 7 indicates the currently active drive mode enabled, except for ECO. Continually broadcasted value
//0x3E8 = VehConfig1 (Request Code 0122) message parameters from BCM
//          Byte 1, Bits 1 and 5, contains "Vehicle Line" setting (ie: 01 = Grand Cherokee | 23 = Durango)
//0x3E9 = VehConfig2 (Request Code 0123) message parameters from BCM
//0x3EA = VehConfig3 (Request code 0124) message parameters from BCM (continually broadcast)
//          Byte 3, Bit 6, contains "Vehicle is an SRT" setting (ie: 29 = No | 69 = Yes)
//          Byte 5, Bit 4, contains "Vehicle Brand" setting (ie: 24 = Jeep | 34 = Dodge)
//
//                _____
//                |   |
//                |BCM|
//                |___|
//                 V ^  
//          0x2FA  V ^  
//          0x3E8  V ^  0x3A0
//          0x3E9  V ^  
//          0x3EA  V ^                    
//              ___V_^___  0x3A0   ______   0x3A0  _____
//              |       |< < < < <|      |< < < < <|   |                   
//              |IHS BUS|   can0  |Teensy|  can1   |UC5|
//              |_______|> > > > >|______|> > > > >|___|
//                         0x3E8            0x3E8
//                         0x3E9            0x3E9
//                         0x3EA            0x3EA   
//                         0x2FA            0x2FA
//                               
//
//Bitwise
//    OR operand will always turn the set bits to a '1' and uses | symbol (00101010 | 01000010 = 01101010)
//    XOR operand will flip the bit to the opposite value and uses ^ symbol (00101010 ^ 01000010 = 01101000) 
//    AND NOT operand will always turn the set bits to a '0' and uses the &~ symbols (00101010 &~ 01000010 = 00101000)
//    

//***************************** DEFINITIONS ***************************************************************************
#include <FlexCAN_T4.h>                                 // include FlexCAN
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;         // Can0: IHS Bus
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can1;         // Can1: UConnect 5
int Can1_RsPin = 3;                                     // Pin definition for canbus module CAN1 RS pin
int Can2_RsPin = 2;                                     // Pin definition for canbus module CAN2 RS pin
CAN_message_t UC5DriveModeFrame;                        // Frame variable to work with 0x3A0 messages
CAN_message_t UC5ECOButtonFrame;                        // Frame variable to work with 0x302 messages
CAN_message_t VehConfig1Frame;                          // Frame variable to work with 0x3E8 messages
CAN_message_t VehConfig2Frame;                          // Frame variable to work with 0x3E9 messages
CAN_message_t VehConfig3Frame;                          // Frame variable to work with 0x3EA messages

// config values =======================================================================================================
bool Spoof_Vehicle_Line_Durango = true;                 // modify vehicle line to be WD - Durango?
bool Spoof_Vehicle_Brand_Dodge = true;                  // modify vehicle brand to be Dodge?
bool Logging = true;                                    // enable logging to serial?

//************************************** CALLBACK FUNCTIONS ************************************************************
//Handling of frames received on the IHS-bus (Can0) ==================================================================
void gotFrame0(const CAN_message_t &frame) {                 // Callback function to handle messages received from Can0 (IHS)
//  log(frame, 5);                                             // Uncomment to enable logging
  if (frame.id == 0x3E8) {HandleVehConfig1(frame);}          // If Frame ID = 3E8 call VehConfig1 handle function
  else if (frame.id == 0x3E9) {HandleVehConfig2(frame);}     // If Frame ID = 3E9 call VehConfig2 handle function
  else if (frame.id == 0x3EA) {HandleVehConfig3(frame);}     // If Frame ID = 3EA call VehConfig3 handle function
  else {Can1.write(frame);}                                  // not a frame we care about, just proxy to can1
}

//Handling of frames received from the UC5 (Can1) ====================================================================
void gotFrame1(const CAN_message_t &frame) {                 // Callback function to handle messages received from Can1 (UC5)
//  log(frame, 6);                                             // Uncomment to enable logging
  Can0.write(frame);                                         // Forward frame to IHS
}

//**************************************** CAN0 (IHS) FRAME HANDLING FUNCTIONS *******************************************

void HandleVehConfig1(const CAN_message_t &frame) {          // Hanndle VehConfig1 (0x3E8) messages
// log(frame, 1);                                              // Uncomment to enable logging
  VehConfig1Frame = frame;                                   // Make a copy of the original frame
  if (Spoof_Vehicle_Line_Durango == true) {                  // If global variable is set to change Vehicle Line, then...
    VehConfig1Frame.buf[1] = 0x23;                           // set byte 1 to 23
//    VehConfig1Frame.buf[1] = frame.buf[1] &~ 0x22;           // flip bits 1 and 5 to "On" within Byte 1
    Can1.write(VehConfig1Frame);                             // Send modified frame to UC5
//    log(VehConfig1Frame, 7);                                 // Unomment to enable logging
  } 
  else {                                                     // else global variable is NOT set to change Vehicle Line, then...
    Can1.write(frame);                                       // forward original unmodified frame to UC5
//    log(frame, 5);                                           // uncomment to enable logging
  }
}

void HandleVehConfig2(const CAN_message_t &frame) {          // Handle VehConfig2 (0x3E9) messages, if needed. 
// log(frame, 1);                                              // uncomment to enable logging
  VehConfig2Frame = frame;                                   //Make a copy of the original frame  
  Can1.write(frame);                                         // Forward original, unmodified frame to UC5
// log(frame, 5);                                              // uncomment to enable logging
}

void HandleVehConfig3(const CAN_message_t &frame) {          // Handle VehConfig3 (0x3EA) messages  
// log(frame, 1);                                              // uncomment to enable logging
  VehConfig3Frame = frame;                                   // Make a copy of the original frame  
  if (Spoof_Vehicle_Brand_Dodge == true) {                   // If global variable is set to chaneg Vehicle Brand, then...
    VehConfig3Frame.buf[5] = 0x34;                           // Set Byte 5 of the modified frame to value 34
//    VehConfig3Frame.buf[5] = frame.buf[5] &~ 0x10;           // flip bit 4 "On" within Byte 5
    Can1.write(VehConfig3Frame);                             // Send modified frame to UC5
//    log(frame, 7);                                           // uncomment to enable logging
  }
  else {
    Can1.write(frame);                                       // Else just forward the original, unmodified frame to UC5
//    log(frame, 5);                                           // uncomment to enable logging
  }
}

//**************************************** INITIALIZATION ****************************************************************
void setup() {
  Serial.println("[wk2-uc5-srt] welcome");                   // display serial welcome message

  // set up pins to act as transceivers
  pinMode(Can1_RsPin, OUTPUT);
  pinMode(Can2_RsPin, OUTPUT);

  // set pins to low state to enable normal mode
  digitalWrite(Can1_RsPin, LOW);
  digitalWrite(Can2_RsPin, LOW);

  // set up Can0
  Can0.begin();
  Can0.setBaudRate(125000);
  Can0.setMaxMB(9);
  Can0.enableFIFO();
  Can0.enableFIFOInterrupt();
  Can0.onReceive(FIFO, gotFrame0);            //Register callback for Can0
  Can0.mailboxStatus();

  // set up Can1
  Can1.begin();
  Can1.setBaudRate(125000);
  Can1.setMaxMB(9);
  Can1.enableFIFO();
  Can1.enableFIFOInterrupt();
  Can1.onReceive(FIFO, gotFrame1);            //Register callback for Can1  
  Can1.mailboxStatus();
}

//**************************************** MAIN LOOP *********************************************************************
void loop() {
  // noop
}

// ************************** SERIAL PRINT FUNCTION **********************************************************************
void log(CAN_message_t Pframe, int src) {
  if (Logging == true) {
    if (src == 1) {
      Serial.print("[CAN0:rx] ");
    } else if (src == 2) {
      Serial.print("[CAN1:rx] ");
    } else if (src == 3) {
      Serial.print("[CAN0:tx] ");
    } else if (src == 4) {
      Serial.print("[CAN1:tx] ");
    } else if (src == 5) {
      Serial.print("[CAN0:fw->CAN1] ");
    } else if (src == 6) {
      Serial.print("[CAN1:fw->CAN0] ");
    } else if (src == 7) {
      Serial.print("[Can0:spoof->CAN1] ");
    }
    Serial.print("ID: 0x");
    Serial.print(Pframe.id, HEX);
    Serial.print("Len: ");
    Serial.print(Pframe.len);
    Serial.print("Data: 0x");
    for (int count = 0; count < Pframe.len; count++) {
      Serial.print(Pframe.buf[count], HEX);
      Serial.print(" ");
    }
    Serial.print("\n");
  }
}