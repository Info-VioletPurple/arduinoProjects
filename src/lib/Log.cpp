/*
 * Log.cpp
 *
 *  Created on: Jul 1, 2017
 *      Author: aaneja
 */

#include"Log.h"

void Log::info(String str) {
	Serial.println(str);
}

void Log::info(const __FlashStringHelper *ifsh) {
	Serial.println(ifsh);
}

void Log::event(uint32_t store_id, uint32_t device_id, uint32_t member_id,
		MFRC522::Uid uid, String event, String message) {

}

void Log::event(uint32_t store_id, uint32_t device_id, String event,
		String message) {

}

