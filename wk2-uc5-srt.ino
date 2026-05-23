// thanks stoop for your help! :)

#include <FlexCAN_T4.h>                                 // include FlexCAN

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;         // Can0: IHS Bus from vehicle
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can1;         // Can1: Out to UConnect 5
int Can1_Rs_Pin = 3;                                    // pin definition for CAN1
int Can2_Rs_Pin = 2;                                    // pin definition for CAN2

// config values
bool Spoof_Vehicle_Line_Durango = true;                 // modify vehicle line to be WD - Durango?
bool Spoof_Vehicle_Brand_Dodge = true;                  // modify vehicle brand to be Dodge?
bool Logging = true;                                    // enable logging to serial?

/**
 * A message was received from the vehicle to be sent to the UConnect.
 */
void onRecievedCan0(const CAN_message_t &frame) {
  // perform filtering
  if (frame.id == 0x3E8) {
    // VehConfig1
    handleSpoofVehConfig1(frame);
  } else if (frame.id == 0x3E9) {
    // VehConfig2
    handleSpoofVehConfig2(frame);
  } else if (frame.id == 0x3EA) {
    // VehConfig3
    handleSpoofVehConfig3(frame);
  } else {
    // not a frame we care about, just proxy to can1
    Can1.write(frame);
    //log(frame, 5);
  }
}

/**
 * A message was received from the UConnect to be sent to the vehicle.
 */
void onReceivedCan1(const CAN_message_t &frame) {
  // just proxy
  Can0.write(frame);
  //log(frame, 6);
}

/**
 * Spoof the VehConfig1 Vehicle Line, if enabled.
 */
void handleSpoofVehConfig1(const CAN_message_t &frame) {
  log(frame, 1);
  if (Spoof_Vehicle_Line_Durango == true) {
    // set byte 1 to 23
    frame.buf[1] = 0x23;
    Can1.write(frame);
    log(frame, 7);
  } else {
    Can1.write(frame);
    log(frame, 5);
  }
}

/**
 * Spoof the VehConfig2 values, if needed. 
 */
void handleSpoofVehConfig2(const CAN_message_t &frame) {
  log(frame, 1);

  // just proxy
  Can1.write(frame);
  log(frame, 5);
}

/**
 * Spoof the VehConfig3 Vehicle Brand, if enabled.
 */
void handleSpoofVehConfig3(const CAN_message_t &frame) {
  log(frame, 1);

  if (Spoof_Vehicle_Brand_Dodge == true) {
    frame.buf[5] = 0x34;
    Can1.write(frame);
    log(frame, 7);
  } else {
    Can1.write(frame);
    log(frame, 5);
  }
}

void log(CAN_message_t frame, int src) {
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

void setup() {
  // display serial welcome message
  Serial.println("[wk2-uc5-srt] welcome");

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

  // Add interceptor to Can0
  Can0.onReceive(FIFO, onRecievedCan0);

  Can0.mailboxStatus();

  // set up Can1
  Can1.begin();
  Can1.setBaudRate(125000);
  Can1.setMaxMB(9);
  Can1.enableFIFO();
  Can1.enableFIFOInterrupt();

  // Add interceptor to Can1
  Can1.onReceive(FIFO, onReceivedCan1);
  Can1.mailboxStatus();
}

void loop() {
  // noop
}
