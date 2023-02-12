/**
 * A Utility Class for taking care of operations on the RFID Play Card.
 * This class uses MFRC522 library to manage the card.
 */

#ifndef CardUtil_h
#define CardUtil_h

#include <MFRC522.h>
#define GLOBAL_SECTOR   0           // Open Sector, Default key read
#define PLAYER_SECTOR   6           // Data sector for players 1
#define MEMBER_SECTOR   7           // Data sector for members 2
#define SEQ_GAME_SECTOR   8			// Data sector for Seq Game 3

class CardUtil {
public:
	/**
	 * Constructor. Takes MFRC522 object as input.
	 */
	CardUtil(MFRC522 _mfrc522);

	// Status codes from the functions in this class.
	enum StatusCode
		: byte {
			STATUS_OK,	// Success
		STATUS_FAILURE,	// Failure
		STATUS_INSUFFICIENT_POINTS,	// Insufficient points.
		STATUS_INSUFFICIENT_REWARDS,	// Insufficient rewards.
		STATUS_ERROR_WITH_CARD,	// Error communicating with the card.
	};
	//Status returned from the functions in this class.
	typedef struct {
		StatusCode code;
		MFRC522::StatusCode mfrc522StatusCode;
		int32_t currentPoints;
		int32_t currentRewards;
		int32_t currentSeq;
	} Status;

	//Global Operations
	/**
	 * Halt the card communication.
	 */
	Status stop();

	/**
	 * Configures the card by writing Global information on the card and setting proper key access.
	 * This function should be called once on the card after it is received from manufacturer.
	 * Initially 0 points are loaded by default. 
	 */
	Status configure();

	/**
	 * Configures the card by writing Global information on the card and setting proper key access.
	 * This function should be called once on the card after it is received from manufacturer.
	 */
	Status configure(int32_t numPoints	//Points to be loaded initially.
			);

	/**
	 * Configures the card by writing Global information on the card and setting proper key access.
	 * This function should be called once on the card after it is received from manufacturer.
	 */
	Status configure(int32_t numPoints,			//Points to be loaded initially.
			MFRC522::MIFARE_Key* auth_key, //Auth key to be used for authentication
			MFRC522::PICC_Command cmd //cmd to specify whether to use Key A or B for auth.
			);

	/**
	 * Configures the card by writing Global information on the card and setting proper key access.
	 * This function should be called in order to re-use a card.
	 */
	Status reset(int32_t numPoints	//Points to be loaded initially.
			);

	/**
	 * Returns the current status of the card.
	 */
	Status checkStatus();

	// Player Info related operations

	//Points related operations.
	/**
	 * Returns the current Points.
	 */
	Status getPoints();

	/**
	 * Adds the numPoints to the currentPoints in the card.
	 * The card is updated with the new currentPoints.
	 */
	Status addPoints(int32_t numPoints	//Points to be added.
			);

	/**
	 * Charges the card with the given points.
	 * The function checks if the current balance is greater than the requested numPoints.
	 * If not, then returns STATUS_INSUFFICIENT_POINTS, else updates the current Points deducting numPoints and saves to the card.
	 * updated points are returned in the status. 
	 */
	Status chargePoints(int32_t numPoints	//Points to be charged.
			);

	//Reward related operations.	

	/**
	 * Adds the numRewards to the currentRewards in the card.
	 * The card is updated with the new currentRewards.
	 */
	Status addRewards(int32_t numRewards	//Rewards to be added.
			);

	/**
	 * Returns the current Rewards.
	 */
	Status getRewards();

	/**
	 * Charges the card with the given Rewards.
	 * The function checks if the current balance is greater than the requested numRewards.
	 * If not, then returns STATUS_INSUFFICIENT_REWARDS, else updates the current rewards deducting numRewards and saves to the card.
	 * updated rewards are returned in the status. 
	 */
	Status chargeRewards(int32_t numRewards	//Rewards to be charged.
			);

	//Sequence game related operations.	

	/**
	 * Initialize the sequence game.
	 */
	Status initSequence(byte* sequence,	//16 byte Sequence to be followed for the game.
			int32_t numRewards	//Rewards to win at the end of the game.
			);

	/**
	 * Checks the next byte in the sequence against the given input.
	 * If the input matches the next in sequence, then current sequence is updated and success returned.
	 * An error is returned if the next byte in sequence doesn't match the given input.
	 * If this is the last in sequence, the game is finished and rewards awarded.
	 */
	Status checkSequence(byte next	//Next value in sequence to be checked.
			);

	//Membership related operations.
	/**
	 */
protected:
	static const int32_t secret_key_version = 1;
	MFRC522::MIFARE_Key secret_key;				//Secret key
	MFRC522::MIFARE_Key default_key;			//Default Key
	MFRC522 mfrc522;							//MFRC522 instance
	byte trailerBlockData[16];
	const MFRC522::MIFARE_Key secret_keys[1] = {//Secret key Array containing all secret keys.
			secret_key };
	static const byte secret_key_array_v1[6] = {		//Secret key
			0xab, 0x28, 0x29, 0x44, 0x2b, 0xFF };

};
#endif
