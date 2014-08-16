/*************************************************************
project: <type project name here>
author: <type your name here>
description: <type what this file does>
*************************************************************/

#include "ArduinoXPlaneAutopilot_main.h"

/*
TXBuffer : DATA + \0 + (long)idx + (float)*8 + (longidx + (float)*8 + ... 

RXBuffer :
[0..4] : DATA@

[5..8] : 3L (idx_speed)
[9..12] : Vind KIAS
[13..16] : Vind KEAS
[17..20] : Vtrue KTAS
[21..24] : Vtrue KTGS
[25..28] : ---
[29..32] : Vind MPH
[33..36] : Vtrue MPHAS
[37..40] : Vtrue MPGHS

[41..44] : 5L (idx_mach)
[45..48] : Mach
[49..52] : ---
[53..56] : VVI
[57..60] : ---
[61..64] : G norm
[65..68] : G axial
[69..72] : G side
[73..76] : --

[77..80] : 18L (throttle)
[81..84] : alpha
[85..88] : beta
[89..112] : ...

[113..116] : 25L (throttle)
[117..120] : thr1
[121..124] : thr2

*/
void setup() {
	// Setup Serial
	Serial.begin(19200);
	while (!Serial) { ; }

	// Setup Ethernet
	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure Ethernet using DHCP");
		for(;;) ;
	}
	
	// print local IP address:
	Serial.print("My IP address: ");
	for (byte thisByte = 0; thisByte < 4; thisByte++) {
		Serial.print(Ethernet.localIP()[thisByte], DEC);
		Serial.print(".");
	}
	Serial.println();
	
	// Listen to local port
	udp.begin(localPort);
	
	// Setup TX Buffer
	memset(&TXBuffer, 0, UDP_TX_PACKET_MAX_SIZE);
	TXBuffer[0] = 'D';
	TXBuffer[1] = 'A';
	TXBuffer[2] = 'T';
	TXBuffer[3] = 'A';
	TXBuffer[4] = (char)0;

	memcpy(&TXBuffer[5], &idx_thr, sizeof(idx_thr));	// 25L
	memcpy(&TXBuffer[9], &fzero, sizeof(fzero));		// thro1
	memcpy(&TXBuffer[13], &fzero, sizeof(fzero));		// thro2
	memcpy(&TXBuffer[17], &fzero, sizeof(fzero));		// thro3
	memcpy(&TXBuffer[21], &fzero, sizeof(fzero));		// thro4
	memcpy(&TXBuffer[25], &fzero, sizeof(fzero));		// thro5
	memcpy(&TXBuffer[29], &fzero, sizeof(fzero));		// thro6
	memcpy(&TXBuffer[33], &fzero, sizeof(fzero));		// thro7
	memcpy(&TXBuffer[37], &fzero, sizeof(fzero));		// thro8
	
	memcpy(&TXBuffer[41], &idx_joy, sizeof(idx_joy));	// 8L
	memcpy(&TXBuffer[45], &fzero, sizeof(fzero));		// elev
	memcpy(&TXBuffer[49], &fzero, sizeof(fzero));		// ailrn
	memcpy(&TXBuffer[53], &fzero, sizeof(fzero));		// ruddr
	memcpy(&TXBuffer[57], &fzero, sizeof(fzero));		// --
	memcpy(&TXBuffer[61], &fzero, sizeof(fzero));		// nwhel
	memcpy(&TXBuffer[65], &fzero, sizeof(fzero));		// --
	memcpy(&TXBuffer[69], &fzero, sizeof(fzero));		// --
	memcpy(&TXBuffer[73], &fzero, sizeof(fzero));		// --

	speedPID.SetMode(AUTOMATIC);
	speedPID.SetOutputLimits(0.0,1.0);
	speedPID.SetSampleTime(10);
	speedPID_Target = 120.0f;

	VVIPID.SetMode(AUTOMATIC);
	VVIPID.SetOutputLimits(0.0, 8.0);
	VVIPID.SetSampleTime(10);
	VVIPID_Target = 000.0f;

	
	AoAPID.SetMode(AUTOMATIC);
	AoAPID.SetOutputLimits(-1.0, 1.0);
	AoAPID.SetSampleTime(10);
	AoAPID_Target = 2.5f;

}

void loop() {

	int packetSize = udp.parsePacket();
	bool sendPacket = true;
	bool printDebug = true;

	if(packetSize != 0) {
		udp.read(RXBuffer, UDP_TX_PACKET_MAX_SIZE);

		speed = (float*)&RXBuffer[9];
		VVI = (float*)&RXBuffer[53];
		AoA = (float*)&RXBuffer[81];
		throttle = (float*)&RXBuffer[117];

		if(printDebug) {
			Serial.print("speed : "); Serial.print(*speed);
			Serial.print(" / VVI : "); Serial.print(*VVI);
			Serial.print(" / throttle : "); Serial.print(*throttle);
			Serial.print(" / AoA : "); Serial.print(*AoA);
			Serial.println();
		}
	
		speedPID_In = *speed;
		speedPID_Out = *throttle;	// May be useless
		speedPID.Compute();

		VVIPID_In = *VVI;
		VVIPID.Compute();
		
		AoAPID_Target = VVIPID_Out;
		AoAPID_In = *AoA;
		AoAPID.Compute();
		Serial.println( VVIPID_Out);
		
		

		memcpy(&TXBuffer[9], &speedPID_Out, sizeof(speedPID_Out));
		memcpy(&TXBuffer[13], &speedPID_Out, sizeof(speedPID_Out));
		
		memcpy(&TXBuffer[45], &AoAPID_Out, sizeof(AoAPID_Out));

		if(sendPacket) {
			udp.beginPacket(udp.remoteIP(), 49000);
			udp.write((uint8_t*)TXBuffer, 77);
			udp.endPacket();
		}
	}
}
