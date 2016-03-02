/*
 * edystone.h
 *
 *  Created on: Dec 12, 2015
 *      Author: daber
 */

#ifndef EDDYSTONE_H_
#define EDDYSTONE_H_

#include <Arduino.h>
#define HTTP_WWW "http://www."
#define HTTPS_WWW "https://www."
#define HTTP "http://"
#define HTTPS "https://"
#define UID_FRAME 0
#define URL_FRAME 0x10

class EdystoneBeacon {

public:
	enum Mode {
		MODE_DISABLED, MODE_UID, MODE_URL
	};

	EdystoneBeacon(int8_t transmit_power);

	virtual
	~EdystoneBeacon();

	void
	init();

	void
	setBroadcastInterval(uint16_t interval_ms);

	void
	loop();

//  void
//  setInterval();
	void
	broadcastURL(String url);

	void
	broadcastID(uint8_t nid[10], uint8_t bid[6]);

	Mode
	getOperationMode();

private:
	void
	releaseAlocatedURLFrame();
	void
	ble_setup();
	void
	ble_loop();
	uint8_t
	getFrameSize();
	uint8_t*
	getFramePointer();
	struct EdystoneId {
		uint8_t frame_id;
		int8_t txPower;
		uint8_t nid[10];
		uint8_t bid[6];
		uint8_t _reserved[2];
	};

	struct EdystoneURLHeader {
		uint8_t frame_id;
		int8_t txPower;
		uint8_t urlScheme;
	};

	Mode mode;
	int8_t transmit_power;
	String url;
	EdystoneURLHeader* urlFrame;
	EdystoneId idFrame;
	const int8_t idFrameSize;
	int8_t urlFrameSize;
	uint16_t interval;

};

#endif /* EDDYSTONE_H_ */
