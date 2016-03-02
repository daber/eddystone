/*
 * edystone.cpp
 *
 *  Created on: Dec 13, 2015
 *      Author: daber
 */

#include "Eddystone.h"

#include <SPI.h>
#include <EEPROM.h>
#include <lib_aci.h>
#include <aci_setup.h>

#include "boards/nemoboard_nfr8001_pins.h"

#include "services.h"
const char* schemes[] = { HTTPS_WWW, HTTP_WWW, HTTPS, HTTP };

const uint8_t schemesCodes[] = { 1, 0, 3, 2 };

#ifdef SERVICES_PIPE_TYPE_MAPPING_CONTENT
static services_pipe_type_mapping_t services_pipe_type_mapping[NUMBER_OF_PIPES] =
SERVICES_PIPE_TYPE_MAPPING_CONTENT;
#else
#define NUMBER_OF_PIPES 0
		static services_pipe_type_mapping_t * services_pipe_type_mapping = NULL;
#endif
		static const hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] PROGMEM = SETUP_MESSAGES_CONTENT;

// aci_struct that will contain
// total initial credits
// current credit
// current state of the aci (setup/standby/active/sleep)
// open remote pipe pending
// close remote pipe pending
// Current pipe available bitmap
// Current pipe closed bitmap
// Current connection interval, slave latency and link supervision timeout
// Current State of the the GATT client (Service Discovery)
// Status of the bond (R) Peer address
		static struct aci_state_t aci_state;
		static hal_aci_evt_t aci_data;
		static hal_aci_data_t aci_cmd;

		void __ble_assert(const char *file, uint16_t line)
		{
			Serial.print("ERROR ");
			Serial.print(file);
			Serial.print(": ");
			Serial.print(line);
			Serial.print("\n");
			while(1);
		}
		void EdystoneBeacon::ble_setup(void)
		{
			//Wait until the serial port is available (useful only for the Leonardo)
			//As the Leonardo board is not reseted every time you open the Serial Monitor
#if defined (__AVR_ATmega32U4__)
		while(!Serial)
		{}
		delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
#elif defined(__PIC32MX__)
		delay(1000);
#endif
		Serial.println(F("ble setup"));

		if (NULL != services_pipe_type_mapping)
		{
			aci_state.aci_setup_info.services_pipe_type_mapping = &services_pipe_type_mapping[0];
		}
		else
		{
			aci_state.aci_setup_info.services_pipe_type_mapping = NULL;
		}
		aci_state.aci_setup_info.number_of_pipes = NUMBER_OF_PIPES;
		aci_state.aci_setup_info.setup_msgs = (hal_aci_data_t*) setup_msgs;
		aci_state.aci_setup_info.num_setup_msgs = NB_SETUP_MESSAGES;

		/*
		 Tell the ACI library, the MCU to nRF8001 pin connections.
		 The Active pin is optional and can be marked UNUSED
		 */
		aci_state.aci_pins.board_name = BOARD_DEFAULT; //See board.h for details
		aci_state.aci_pins.reqn_pin = NFR8001_REQ_PIN;
		aci_state.aci_pins.rdyn_pin = NFR8001_RDY_PIN;
		aci_state.aci_pins.mosi_pin = MOSI;
		aci_state.aci_pins.miso_pin = MISO;
		aci_state.aci_pins.sck_pin = SCK;

		aci_state.aci_pins.spi_clock_divider = SPI_CLOCK_DIV8;//SPI_CLOCK_DIV8  = 2MHz SPI speed
		//SPI_CLOCK_DIV16 = 1MHz SPI speed

		aci_state.aci_pins.reset_pin = NFR8001_RST_PIN;
		aci_state.aci_pins.active_pin = UNUSED;
		aci_state.aci_pins.optional_chip_sel_pin = UNUSED;

		aci_state.aci_pins.interface_is_interrupt = NFR8001_USE_INTERRUPT;
		aci_state.aci_pins.interrupt_number = digitalPinToInterrupt(NFR8001_RDY_PIN);

		//We reset the nRF8001 here by toggling the RESET line connected to the nRF8001
		//and initialize the data structures required to setup the nRF8001
		lib_aci_init(&aci_state, false);
	}

	void EdystoneBeacon::ble_loop()
	{
		static bool setup_required = false;

		// We enter the if statement only when there is a ACI event available to be processed
		if (lib_aci_event_get(&aci_state, &aci_data))
		{
			aci_evt_t * aci_evt;
			aci_evt = &aci_data.evt;
			switch(aci_evt->evt_opcode)
			{
				/**
				 As soon as you reset the nRF8001 you will get an ACI Device Started Event
				 */
				case ACI_EVT_DEVICE_STARTED:
				{
					aci_state.data_credit_available = aci_evt->params.device_started.credit_available;
					switch(aci_evt->params.device_started.device_mode)
					{
						case ACI_DEVICE_SETUP:
						/**
						 When the device is in the setup mode
						 */
						Serial.println(F("Evt Device Started: Setup"));
						setup_required = true;
						break;

						case ACI_DEVICE_STANDBY:
						Serial.println(F("Evt Device Started: Standby"));
						//See ACI Broadcast in the data sheet of the nRF8001
						Serial.println("Mode of operation");
						Serial.println(mode);
						if(mode != MODE_DISABLED)
						{
							Serial.println("Starting broadcasting ");
							Serial.println(mode);
							lib_aci_set_local_data(&aci_state,PIPE_EDYSTONE_EDYSTONE_DATA_SET,getFramePointer(),getFrameSize());
							lib_aci_open_adv_pipe(PIPE_EDYSTONE_EDYSTONE_DATA_BROADCAST);
							lib_aci_broadcast(0/*in seconds*/, interval/*in 0.625 ms*/);

						}
						//While broadcasting (non_connectable) interval of 100ms is the minimum possible
						//To stop the broadcasting before the timeout use the
						//lib_aci_radio_reset to soft reset the radio
						//See ACI RadioReset in the datasheet of the nRF8001
						break;
					}
				}
				break; //ACI Device Started Event

				case ACI_EVT_CMD_RSP:
				//If an ACI command response event comes with an error -> stop
				if (ACI_STATUS_SUCCESS != aci_evt->params.cmd_rsp.cmd_status)
				{
					//ACI ReadDynamicData and ACI WriteDynamicData will have status codes of
					//TRANSACTION_CONTINUE and TRANSACTION_COMPLETE
					//all other ACI commands will have status code of ACI_STATUS_SCUCCESS for a successful command
					Serial.print(F("ACI Command "));
					Serial.println(aci_evt->params.cmd_rsp.cmd_opcode, HEX);
					Serial.println(F("Evt Cmd respone: Error. Arduino is in an while(1); loop"));
					while (1);
				}
				break;

				case ACI_EVT_CONNECTED:
				Serial.println(F("Evt Connected"));
				break;

				case ACI_EVT_PIPE_STATUS:
				Serial.println(F("Evt Pipe Status"));
				break;

				case ACI_EVT_DISCONNECTED:
				if (ACI_STATUS_ERROR_ADVT_TIMEOUT == aci_evt->params.disconnected.aci_status)
				{
					Serial.println(F("Broadcasting timed out"));
				}
				else
				{
					Serial.println(F("Evt Disconnected. Link Loss"));
				}
				break;

				case ACI_EVT_DATA_RECEIVED:
				Serial.print(F("Data received on Pipe #: 0x"));
				Serial.println(aci_evt->params.data_received.rx_data.pipe_number, HEX);
				Serial.print(F("Length of data received: 0x"));
				Serial.println(aci_evt->len - 2, HEX);
				break;

				case ACI_EVT_HW_ERROR:
				Serial.println(F("HW error: "));
				Serial.println(aci_evt->params.hw_error.line_num, DEC);
				for(uint8_t counter = 0; counter <= (aci_evt->len - 3); counter++)
				{
					Serial.write(aci_evt->params.hw_error.file_name[counter]); //uint8_t file_name[20];
				}
				Serial.println();
				break;
			}
		}
		else
		{
			//Serial.println(F("No ACI Events available"));
			// No event in the ACI Event queue
			// Arduino can go to sleep now
			// Wakeup from sleep from the RDYN line
		}

		/* setup_required is set to true when the device starts up and enters setup mode.
		 * It indicates that do_aci_setup() should be called. The flag should be cleared if
		 * do_aci_setup() returns ACI_STATUS_TRANSACTION_COMPLETE.
		 */
		if(setup_required)
		{
			if (SETUP_SUCCESS == do_aci_setup(&aci_state))
			{
				setup_required = false;
			}
		}
	}

	EdystoneBeacon::EdystoneBeacon (int8_t txPower): mode (MODE_DISABLED),
transmit_power (0), urlFrame (0),idFrameSize(sizeof(EdystoneId)), urlFrameSize (0)
{
	memset(&idFrame,0,idFrameSize);
	transmit_power = txPower +41;
}

EdystoneBeacon::~EdystoneBeacon() {
	releaseAlocatedURLFrame();
}

void EdystoneBeacon::releaseAlocatedURLFrame() {
	if (urlFrame != 0) {
		free(urlFrame);
		urlFrame = 0;
		urlFrameSize = 0;
	}
}

void EdystoneBeacon::broadcastID(uint8_t nid[10], uint8_t bid[6]) {
	releaseAlocatedURLFrame();
	mode = MODE_UID;
	idFrame.frame_id = UID_FRAME;
	idFrame.txPower = transmit_power;
	memcpy(idFrame.bid, bid, sizeof(idFrame.bid));
	memcpy(idFrame.nid, nid, sizeof(idFrame.nid));
}

uint8_t EdystoneBeacon::getFrameSize() {
	switch (mode) {
	case MODE_DISABLED:
		return 0;
	case MODE_UID:
		return idFrameSize;
	case MODE_URL:
		return urlFrameSize;
	}
	return 0;
}

EdystoneBeacon::Mode EdystoneBeacon::getOperationMode() {
	return mode;
}

uint8_t*
EdystoneBeacon::getFramePointer() {
	switch (mode) {
	case MODE_UID:
		return (uint8_t*) &idFrame;
	case MODE_URL:
		return (uint8_t*) urlFrame;
	default:
		return 0;
	}
	return 0;
}

void EdystoneBeacon::broadcastURL(String url) {
	releaseAlocatedURLFrame();
	mode = MODE_URL;
	uint8_t code = 0;

	for (uint8_t i = 0; i < sizeof(schemes) / sizeof(const char*); i++) {
		if (url.startsWith(schemes[i])) {
			code = schemesCodes[i];
			url.remove(0, strlen(schemes[i]));
		}
	}
	const char* encodedUrl = url.c_str();
	int len = strlen(encodedUrl);
	size_t header_size = sizeof(EdystoneURLHeader);
	size_t packet_size = header_size + len;

	urlFrame = (EdystoneURLHeader*) malloc(packet_size);

	urlFrame->frame_id = URL_FRAME;
	urlFrame->txPower = transmit_power;
	urlFrame->urlScheme = code;

	char* encURLptr = ((char*) urlFrame) + sizeof(EdystoneURLHeader);
	memcpy(encURLptr, encodedUrl, strlen(encodedUrl));
	urlFrameSize = packet_size;
}

void EdystoneBeacon::init() {
	ble_setup();
}

void EdystoneBeacon::loop() {
	ble_loop();
}

void EdystoneBeacon::setBroadcastInterval(uint16_t interval_ms) {
	//32 - 16384
	//0.625 unit ms;
	interval = (uint16_t) interval_ms / 0.625;
	if (interval < 32) {
		interval = 32;
	} else if (interval > 16384) {
		interval = 16384;
	}
}

