/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

/**	@defgroup serial Serial
 *	@brief	Serial interface. All serial implementations should extend this one
 *  @{
 */

#ifndef SMINGCORE_SERIALSTREAM_H_
#define SMINGCORE_SERIALSTREAM_H_

#include "../Wiring/Stream.h"

class SerialStream: public Stream
{
public:
	/*
	* @brief Returns the location of the searched character
	* @param char c - character to search for
	* @retval size_t -1 if not found 0 or positive number otherwise
	*/
	virtual size_t indexOf(char c) = 0;
};

/** @} */
#endif /* SMINGCORE_SERIALSTREAM_H_ */
