/**
 * A Utility Class for taking care of operations on the RFID Play Card.
 * This class uses MFRC522 library to manage the card.
 */

#include "CardUtil.h"
extern HardwareSerial Serial;

//Constructor
CardUtil::CardUtil(MFRC522 _mfrc522) {
	mfrc522 = _mfrc522;
	// Prepare the default key (used both as key A and as key B)
	// using FFFFFFFFFFFFh which is the default at chip delivery from the factory
	for (byte i = 0; i < 6; i++) {
		default_key.keyByte[i] = 0xFF;
	}
	// Prepare the secret key (used as key B)
	for (int i = 0; i < 6; ++i) {
		secret_key.keyByte[i] = secret_key_array_v1[i];
	}
	//Trailer Block
	//secret key A
	for (byte i = 0; i < 6; i++) {
		trailerBlockData[i] = 0xFF;
	}
	//access bits
	//Block Access 110
	//Trailer Access 011
	//0 1 1 never key B key A|B key B never key B
	//1 1 0 key A|B key B key B key A|B value block[1]
	//C1  C2  C3
	//0   1   1
	//1   1   0
	//1   1   0
	//1   1   0
	//08 77 8F  
	trailerBlockData[6] = 0x08;
	trailerBlockData[7] = 0x77;
	trailerBlockData[8] = 0x8F;
	trailerBlockData[9] = 0x69;
	//default Secret key B
	for (byte i = 10; i < 16; i++) {
		trailerBlockData[i] = secret_key_array_v1[i - 10];
	}
}
/**
 *  Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
	for (byte i = 0; i < bufferSize; i++) {
		Serial.print(buffer[i] < 0x10 ? " 0" : " ");
		Serial.print(buffer[i], HEX);
	}
}

CardUtil::Status CardUtil::stop() {
	//Finish
	// Halt PICC
	mfrc522.PICC_HaltA();
	// Stop encryption on PCD
	mfrc522.PCD_StopCrypto1();
}

CardUtil::Status CardUtil::configure() {
	return configure(0);
}

CardUtil::Status CardUtil::configure(int32_t numPoints) {
	return configure(numPoints, &default_key, MFRC522::PICC_CMD_MF_AUTH_KEY_A);
}

CardUtil::Status CardUtil::configure(int32_t numPoints,
		MFRC522::MIFARE_Key* auth_key, MFRC522::PICC_Command cmd) {
	Status returnStatus;
	//Global Info
	byte trailerBlock = GLOBAL_SECTOR * 4 + 3;
	byte status;
	// Authenticate using global_key as Key A
	Serial.println(F("Authenticating using key A..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
			trailerBlock, &default_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	byte blockAddr = 1;
	//Represent the today's date here. For debugging in future.
	byte dataBlock[] = { 0x06, 0x01, 0x20, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	// Write date and version
	Serial.print(F("Writing date into block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	dump_byte_array(dataBlock, 16);
	Serial.println();
	status = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println();

	blockAddr++;
	Serial.print(F("Writing key version into block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_SetValue(blockAddr, secret_key_version);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(secret_key_version);

	//Player Info
	trailerBlock = PLAYER_SECTOR * 4 + 3;
	// Authenticate using auth_key
	Serial.print(F("Authenticating using "));
	Serial.println(cmd);
	status = mfrc522.PCD_Authenticate(cmd, trailerBlock, auth_key,
			&(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	blockAddr = PLAYER_SECTOR * 4;

	// Write numPoints
	Serial.print(F("Writing numPoints into block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_SetValue(blockAddr, numPoints);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(numPoints);

	blockAddr++;
	int32_t numRewards = 0;
	// Write numRewards
	Serial.print(F("Writing numRewards into block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_SetValue(blockAddr, numRewards);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(numRewards);

	//Seq Game Info
	trailerBlock = SEQ_GAME_SECTOR * 4 + 3;
	// Authenticate using auth_key
	Serial.print(F("Authenticating using "));
	Serial.println(cmd);
	status = mfrc522.PCD_Authenticate(cmd, trailerBlock, auth_key,
			&(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	blockAddr = SEQ_GAME_SECTOR * 4;

	// Write Current seq
	int32_t cur_seq = -1;
	Serial.print(F("Writing Current Seq into block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_SetValue(blockAddr, cur_seq);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(cur_seq);

	//Encode all trailer blocks to be secured for Violet's use only.
	trailerBlock = PLAYER_SECTOR * 4 + 3;
	while (trailerBlock <= 64) {
		//Authenticate the sector
		// Authenticate using global_key as Key A
		Serial.print(F("Authenticating using "));
		Serial.println(cmd);
		status = mfrc522.PCD_Authenticate(cmd, trailerBlock, auth_key,
				&(mfrc522.uid));
		if (status != MFRC522::STATUS_OK) {
			Serial.print(F("PCD_Authenticate() failed: "));
			Serial.println(mfrc522.GetStatusCodeName(status));
			returnStatus.mfrc522StatusCode = status;
			returnStatus.code = CardUtil::STATUS_ERROR_WITH_CARD;
			return returnStatus;
		}
		//Rewrite the trailer blocks 
		Serial.print(F("Writing trailer block "));
		Serial.print(trailerBlock);
		Serial.println(F(" ..."));
		//dump_byte_array(dataBlock, 16);Serial.println();
		status = mfrc522.MIFARE_Write(trailerBlock, trailerBlockData, 16);
		if (status != MFRC522::STATUS_OK) {
			Serial.print(F("MIFARE_Write() failed: "));
			Serial.println(mfrc522.GetStatusCodeName(status));
			returnStatus.mfrc522StatusCode = status;
			returnStatus.code = CardUtil::STATUS_ERROR_WITH_CARD;
			return returnStatus;
		}
		Serial.println("******");
		trailerBlock += 4; //Move to next trailer block
	}

	returnStatus.code = STATUS_OK;
	returnStatus.currentPoints = numPoints;
	returnStatus.currentRewards = numRewards;
	returnStatus.currentSeq = cur_seq;

	//Member Info

	return returnStatus;
}

CardUtil::Status CardUtil::reset(int32_t numPoints) {
	Status returnStatus;
	//Global Info
	byte trailerBlock = GLOBAL_SECTOR * 4 + 3;
	byte status;
	// Authenticate using global_key as Key A
	Serial.println(F("Authenticating using key A..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
			trailerBlock, &default_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	byte blockAddr = 2;
	//Read Key Version used last time encoded.
	int32_t key_version = 1;
	status = mfrc522.MIFARE_GetValue(blockAddr, &key_version);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_GetValue() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	//Serial.print(“Key Version Read:”); 
	Serial.println((int) key_version);
	if (!(key_version >= 1
			&& key_version <= (sizeof(secret_keys) / sizeof(secret_keys[0])))) {
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	MFRC522::MIFARE_Key secret_key = secret_keys[key_version - 1];

	returnStatus = configure(numPoints, &secret_key,
			MFRC522::PICC_CMD_MF_AUTH_KEY_B);

	return returnStatus;
}

CardUtil::Status CardUtil::checkStatus() {
	Status returnStatus;
	//Player Info
	byte trailerBlock = PLAYER_SECTOR * 4 + 3;
	byte status;
	// Authenticate using secret_key as Key B
	Serial.println(F("Authenticating using key B..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
			trailerBlock, &secret_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	byte blockAddr = PLAYER_SECTOR * 4;
	// Read numPoints
	int32_t currentPoints = 0;
	Serial.print(F("Reading numPoints from block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_GetValue(blockAddr, &currentPoints);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(currentPoints);

	blockAddr++;
	// Read numRewards
	int32_t currentRewards = 0;
	Serial.print(F("Reading numRewards from block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_GetValue(blockAddr, &currentRewards);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(currentRewards);

	//Sequence Game Info
	trailerBlock = SEQ_GAME_SECTOR * 4 + 3;
	// Authenticate using secret_key as Key B
	Serial.println(F("Authenticating using key B..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
			trailerBlock, &secret_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	blockAddr = SEQ_GAME_SECTOR * 4;
	//Read Current Sequence
	int32_t cur_seq = -1;
	Serial.print(F("Reading cur_seq from block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_GetValue(blockAddr, &cur_seq);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(cur_seq);

	returnStatus.code = STATUS_OK;
	returnStatus.currentPoints = currentPoints;
	returnStatus.currentSeq = cur_seq;
	returnStatus.currentPoints = currentRewards;
	return returnStatus;
}

CardUtil::Status CardUtil::chargePoints(int32_t numPoints) {
	Status returnStatus;
	//Player Info
	byte trailerBlock = PLAYER_SECTOR * 4 + 3;
	byte status;
	// Authenticate using secret_key as Key B
	Serial.println(F("Authenticating using key B..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
			trailerBlock, &secret_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	byte blockAddr = PLAYER_SECTOR * 4;
	// Read numPoints
	int32_t currentPoints = 0;
	Serial.print(F("Reading numPoints from block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_GetValue(blockAddr, &currentPoints);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(currentPoints);

	if (currentPoints < numPoints) {
		Serial.println(
				"Can't play the game as balance is low. Please recharge your card");
		//show error.
		//terminate
		// Halt PICC
		mfrc522.PICC_HaltA();
		// Stop encryption on PCD
		mfrc522.PCD_StopCrypto1();
		returnStatus.code = STATUS_INSUFFICIENT_POINTS;
		return returnStatus;
	} else {
		currentPoints -= numPoints;
		//Write back the updated Points
		Serial.print(F("Writing numPoints into block "));
		Serial.print(blockAddr);
		Serial.println(F(" ..."));
		status = mfrc522.MIFARE_SetValue(blockAddr, currentPoints);
		if (status != MFRC522::STATUS_OK) {
			Serial.print(F("MIFARE_Write() failed: "));
			Serial.println(mfrc522.GetStatusCodeName(status));
			returnStatus.mfrc522StatusCode = status;
			returnStatus.code = STATUS_ERROR_WITH_CARD;
			return returnStatus;
		}
		Serial.println(currentPoints);
		Serial.println("Success. You can Play. Enjoy!!!");
	}

	returnStatus.code = STATUS_OK;
	returnStatus.currentPoints = currentPoints;
	return returnStatus;
}

CardUtil::Status CardUtil::getPoints() {
	Status returnStatus;
	//Player Info
	byte trailerBlock = PLAYER_SECTOR * 4 + 3;
	byte status;
	// Authenticate using secret_key as Key B
	Serial.println(F("Authenticating using key B..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
			trailerBlock, &secret_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	byte blockAddr = PLAYER_SECTOR * 4;
	// Read numPoints
	int32_t currentPoints = 0;
	Serial.print(F("Reading numPoints from block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_GetValue(blockAddr, &currentPoints);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(currentPoints);

	returnStatus.code = STATUS_OK;
	returnStatus.currentPoints = currentPoints;
	return returnStatus;
}

CardUtil::Status CardUtil::addPoints(int32_t numPoints) {
	Status returnStatus;
	//Player Info
	byte trailerBlock = PLAYER_SECTOR * 4 + 3;
	byte status;
	// Authenticate using secret_key as Key B
	Serial.println(F("Authenticating using key B..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
			trailerBlock, &secret_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	byte blockAddr = PLAYER_SECTOR * 4;
	// Read numPoints
	int32_t currentPoints = 0;
	Serial.print(F("Reading numPoints from block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_GetValue(blockAddr, &currentPoints);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(currentPoints);

	currentPoints += numPoints;
	//Write back the updated Points
	Serial.print(F("Writing numPoints into block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_SetValue(blockAddr, currentPoints);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(currentPoints);
	Serial.println("Recharged Successfully");

	returnStatus.code = STATUS_OK;
	returnStatus.currentPoints = currentPoints;
	return returnStatus;
}

CardUtil::Status CardUtil::chargeRewards(int32_t numRewards) {
	Status returnStatus;
	//Player Info
	byte trailerBlock = PLAYER_SECTOR * 4 + 3;
	byte status;
	// Authenticate using secret_key as Key B
	Serial.println(F("Authenticating using key B..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
			trailerBlock, &secret_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	byte blockAddr = PLAYER_SECTOR * 4 + 1;
	// Read numRewards
	int32_t currentRewards = 0;
	Serial.print(F("Reading numRewards from block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_GetValue(blockAddr, &currentRewards);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(currentRewards);

	if (currentRewards < numRewards) {
		Serial.println(
				"Can't reward as balance is low. Please play more games to earn rewards.");
		//show error.
		//terminate
		returnStatus.code = STATUS_INSUFFICIENT_REWARDS;
		return returnStatus;
	} else {
		currentRewards -= numRewards;
		//Write back the updated Rewards
		Serial.print(F("Writing numRewards into block "));
		Serial.print(blockAddr);
		Serial.println(F(" ..."));
		status = mfrc522.MIFARE_SetValue(blockAddr, currentRewards);
		if (status != MFRC522::STATUS_OK) {
			Serial.print(F("MIFARE_Write() failed: "));
			Serial.println(mfrc522.GetStatusCodeName(status));
			returnStatus.mfrc522StatusCode = status;
			returnStatus.code = STATUS_ERROR_WITH_CARD;
			return returnStatus;
		}
		Serial.println(currentRewards);
		Serial.println("Success. Enjoy your reward.");
	}

	returnStatus.code = STATUS_OK;
	returnStatus.currentPoints = currentRewards;
	return returnStatus;
}

CardUtil::Status CardUtil::getRewards() {
	Status returnStatus;
	//Player Info
	byte trailerBlock = PLAYER_SECTOR * 4 + 3;
	byte status;
	// Authenticate using secret_key as Key B
	Serial.println(F("Authenticating using key B..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
			trailerBlock, &secret_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	byte blockAddr = PLAYER_SECTOR * 4 + 1;
	// Read numRewards
	int32_t currentRewards = 0;
	Serial.print(F("Reading numRewards from block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_GetValue(blockAddr, &currentRewards);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(currentRewards);

	returnStatus.code = STATUS_OK;
	returnStatus.currentRewards = currentRewards;
	return returnStatus;
}

CardUtil::Status CardUtil::addRewards(int32_t numRewards) {
	Status returnStatus;
	//Player Info
	byte trailerBlock = PLAYER_SECTOR * 4 + 3;
	byte status;
	// Authenticate using secret_key as Key B
	Serial.println(F("Authenticating using key B..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
			trailerBlock, &secret_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	byte blockAddr = PLAYER_SECTOR * 4 + 1;
	// Read numRewards
	int32_t currentRewards = 0;
	Serial.print(F("Reading numRewards from block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_GetValue(blockAddr, &currentRewards);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(currentRewards);

	currentRewards += numRewards;
	//Write back the updated Rewards
	Serial.print(F("Writing numRewards into block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_SetValue(blockAddr, currentRewards);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(currentRewards);
	Serial.println("Awarded Successfully");

	returnStatus.code = STATUS_OK;
	returnStatus.currentRewards = currentRewards;
	return returnStatus;
}

CardUtil::Status CardUtil::initSequence(byte* sequence, int32_t numRewards) {
	Status returnStatus;
	//Sequence Game Info
	byte trailerBlock = SEQ_GAME_SECTOR * 4 + 3;
	byte status;
	// Authenticate using secret_key as Key B
	Serial.println(F("Authenticating using key B..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
			trailerBlock, &secret_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	//Write Current Sequence
	int32_t cur_seq = 0;
	byte blockAddr = SEQ_GAME_SECTOR * 4;
	Serial.print(F("Writing current sequence into block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_SetValue(blockAddr, cur_seq);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	//Write Game Sequence
	blockAddr++;
	Serial.print(F("Writing sequence into block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_Write(blockAddr, sequence, 16);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	//Write Rewards
	blockAddr++;
	Serial.print(F("Writing rewards into block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_SetValue(blockAddr, numRewards);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	Serial.println("Seq Game initiated. Enjoy!!");

	returnStatus.currentSeq = cur_seq;
	returnStatus.code = STATUS_OK;
	return returnStatus;
}

CardUtil::Status CardUtil::checkSequence(byte next) {
	Status returnStatus;
	//Sequence Game Info
	byte trailerBlock = SEQ_GAME_SECTOR * 4 + 3;
	byte status;
	// Authenticate using secret_key as Key B
	Serial.println(F("Authenticating using key B..."));
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
			trailerBlock, &secret_key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	byte blockAddr = SEQ_GAME_SECTOR * 4;
	//Read Current Sequence
	int32_t cur_seq = -1;
	Serial.print(F("Reading cur_seq from block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_GetValue(blockAddr, &cur_seq);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	Serial.println(cur_seq);
	if (cur_seq == -1) {
		Serial.print(F("Current Sequence not initialized "));
		returnStatus.currentSeq = cur_seq;
		returnStatus.code = STATUS_FAILURE;
		return returnStatus;
	}

	blockAddr++;
	byte sequence[18];
	int32_t numRewards = 0;
	byte size = sizeof(sequence);
	// Read Game Sequence
	Serial.print(F("Reading Game Sequence from block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_Read(blockAddr, sequence, &size);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Read() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}
	dump_byte_array(sequence, 16);
	Serial.println();

	Serial.print(F("check for Next Sequence: Expected:"));
	Serial.print(sequence[cur_seq]);
	Serial.print(F(", Actual:"));
	Serial.print(next);
	Serial.println();
	//check for next sequence
	if (sequence[cur_seq] == next) {
		cur_seq++;
		if (sequence[cur_seq] == 0x00) {
			//terminate game.
			Serial.print(
					F(
							"Correct Sequence. Game Finished. Player Won. Correct Attempts:"));
			Serial.println(cur_seq);
			cur_seq = -1;
			blockAddr++;
			Serial.print(F("Reading numRewards from block "));
			Serial.print(blockAddr);
			Serial.println(F(" ..."));
			status = mfrc522.MIFARE_GetValue(blockAddr, &numRewards);
			if (status != MFRC522::STATUS_OK) {
				Serial.print(F("MIFARE_Write() failed: "));
				Serial.println(mfrc522.GetStatusCodeName(status));
				returnStatus.mfrc522StatusCode = status;
				returnStatus.code = STATUS_ERROR_WITH_CARD;
				return returnStatus;
			}
			Serial.println(numRewards);
		}
	} else {
		//terminate game.
		Serial.print(
				F(
						"Wrong Sequence. Game Finished. Player Lost. Correct Attempts:"));
		Serial.println(cur_seq);
		cur_seq = -1;
	}

	//Write Current Sequence
	blockAddr = SEQ_GAME_SECTOR * 4;
	Serial.print(F("Writing current sequence into block "));
	Serial.print(blockAddr);
	Serial.println(F(" ..."));
	status = mfrc522.MIFARE_SetValue(blockAddr, cur_seq);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName(status));
		returnStatus.mfrc522StatusCode = status;
		returnStatus.code = STATUS_ERROR_WITH_CARD;
		return returnStatus;
	}

	if (numRewards > 0)
		returnStatus = addRewards(numRewards);//award the reward to the player.

	returnStatus.currentSeq = cur_seq;
	returnStatus.code = STATUS_OK;
	return returnStatus;
}
