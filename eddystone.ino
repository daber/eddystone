/* Copyright (c) 2014, Nordic Semiconductor ASA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <avr/sleep.h>

#include "Eddystone.h"
/**
 Include the services_lock.h to put the setup in the OTP memory of the nRF8001.
 This would mean that the setup cannot be changed once put in.
 However this removes the need to do the setup of the nRF8001 on every reset.
 */

EdystoneBeacon beacon(-66);
uint8_t nid[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
uint8_t bid[] = { 1, 2, 3, 4, 5, 6 };

void disableACDC() {
	ADCSRA = ADCSRA & B01111111;
	ACSR = B10000000;
}

void setup(void) {
	Serial.begin(9600);
	disableACDC();
	//beacon.broadcastID(nid,bid);
	beacon.broadcastURL("http://neofonie.de/");
	beacon.setBroadcastInterval(500);
	beacon.init();
}

void sleep_until_interrupted() {
	set_sleep_mode(SLEEP_MODE_IDLE);
	sleep_mode()
	;
}

void loop() {
	beacon.loop();
	sleep_until_interrupted();
}

