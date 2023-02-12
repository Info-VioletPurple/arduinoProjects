/*
 * Log.h
 * Log File for handling logs from the project
 *  Created on: Jul 1, 2017
 *      Author: aaneja
 */
#ifndef Log_h
#define Log_h

#include<MFRC522.h>

class Log {
public:
	/**
	 * Logs info level message.
	 */
	void info(String str);

	/**
	 * Logs info level message.
	 */
	void info(const __FlashStringHelper *ifsh);

	/**
	 * Logs an event captured between a RFID Card and a device reader.
	 */
	void event(uint32_t store_id, uint32_t device_id, uint32_t member_id,
			MFRC522::Uid uid, String event, String message);

	/**
	 * Logs an event captured by a device.
	 */
	void event(uint32_t store_id, uint32_t device_id, String event,
			String message);
};

#endif /* Log_h */
