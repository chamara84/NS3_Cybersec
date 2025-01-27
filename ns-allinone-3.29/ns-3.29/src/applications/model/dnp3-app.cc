/*
 * dnp3-app.cc
 *
 *  Created on: May 1, 2020
 *      Author: chamara
 */
#include "dnp3-app.h"
#define DNP3_MIN_TRANSPORT_LEN 6

#define DNP3_TPDU_MAX  250
#define DNP3_LPDU_MAX  292
using namespace ns3;
using namespace std;


/* CRC look-up table, for computeCRC() below */
static uint16_t crcLookUpTable[256] =
{
		0x0000, 0x365E, 0x6CBC, 0x5AE2, 0xD978, 0xEF26, 0xB5C4, 0x839A,
		0xFF89, 0xC9D7, 0x9335, 0xA56B, 0x26F1, 0x10AF, 0x4A4D, 0x7C13,
		0xB26B, 0x8435, 0xDED7, 0xE889, 0x6B13, 0x5D4D, 0x07AF, 0x31F1,
		0x4DE2, 0x7BBC, 0x215E, 0x1700, 0x949A, 0xA2C4, 0xF826, 0xCE78,
		0x29AF, 0x1FF1, 0x4513, 0x734D, 0xF0D7, 0xC689, 0x9C6B, 0xAA35,
		0xD626, 0xE078, 0xBA9A, 0x8CC4, 0x0F5E, 0x3900, 0x63E2, 0x55BC,
		0x9BC4, 0xAD9A, 0xF778, 0xC126, 0x42BC, 0x74E2, 0x2E00, 0x185E,
		0x644D, 0x5213, 0x08F1, 0x3EAF, 0xBD35, 0x8B6B, 0xD189, 0xE7D7,
		0x535E, 0x6500, 0x3FE2, 0x09BC, 0x8A26, 0xBC78, 0xE69A, 0xD0C4,
		0xACD7, 0x9A89, 0xC06B, 0xF635, 0x75AF, 0x43F1, 0x1913, 0x2F4D,
		0xE135, 0xD76B, 0x8D89, 0xBBD7, 0x384D, 0x0E13, 0x54F1, 0x62AF,
		0x1EBC, 0x28E2, 0x7200, 0x445E, 0xC7C4, 0xF19A, 0xAB78, 0x9D26,
		0x7AF1, 0x4CAF, 0x164D, 0x2013, 0xA389, 0x95D7, 0xCF35, 0xF96B,
		0x8578, 0xB326, 0xE9C4, 0xDF9A, 0x5C00, 0x6A5E, 0x30BC, 0x06E2,
		0xC89A, 0xFEC4, 0xA426, 0x9278, 0x11E2, 0x27BC, 0x7D5E, 0x4B00,
		0x3713, 0x014D, 0x5BAF, 0x6DF1, 0xEE6B, 0xD835, 0x82D7, 0xB489,
		0xA6BC, 0x90E2, 0xCA00, 0xFC5E, 0x7FC4, 0x499A, 0x1378, 0x2526,
		0x5935, 0x6F6B, 0x3589, 0x03D7, 0x804D, 0xB613, 0xECF1, 0xDAAF,
		0x14D7, 0x2289, 0x786B, 0x4E35, 0xCDAF, 0xFBF1, 0xA113, 0x974D,
		0xEB5E, 0xDD00, 0x87E2, 0xB1BC, 0x3226, 0x0478, 0x5E9A, 0x68C4,
		0x8F13, 0xB94D, 0xE3AF, 0xD5F1, 0x566B, 0x6035, 0x3AD7, 0x0C89,
		0x709A, 0x46C4, 0x1C26, 0x2A78, 0xA9E2, 0x9FBC, 0xC55E, 0xF300,
		0x3D78, 0x0B26, 0x51C4, 0x679A, 0xE400, 0xD25E, 0x88BC, 0xBEE2,
		0xC2F1, 0xF4AF, 0xAE4D, 0x9813, 0x1B89, 0x2DD7, 0x7735, 0x416B,
		0xF5E2, 0xC3BC, 0x995E, 0xAF00, 0x2C9A, 0x1AC4, 0x4026, 0x7678,
		0x0A6B, 0x3C35, 0x66D7, 0x5089, 0xD313, 0xE54D, 0xBFAF, 0x89F1,
		0x4789, 0x71D7, 0x2B35, 0x1D6B, 0x9EF1, 0xA8AF, 0xF24D, 0xC413,
		0xB800, 0x8E5E, 0xD4BC, 0xE2E2, 0x6178, 0x5726, 0x0DC4, 0x3B9A,
		0xDC4D, 0xEA13, 0xB0F1, 0x86AF, 0x0535, 0x336B, 0x6989, 0x5FD7,
		0x23C4, 0x159A, 0x4F78, 0x7926, 0xFABC, 0xCCE2, 0x9600, 0xA05E,
		0x6E26, 0x5878, 0x029A, 0x34C4, 0xB75E, 0x8100, 0xDBE2, 0xEDBC,
		0x91AF, 0xA7F1, 0xFD13, 0xCB4D, 0x48D7, 0x7E89, 0x246B, 0x1235
};

/* Append a DNP3 Transport segment to the reassembly buffer.

   Returns:
    DNP3_OK:    Segment queued successfully.
    DNP3_FAIL:  Data copy failed. Segment did not fit in reassembly buffer.
 */
static int DNP3QueueSegment(dnp3_reassembly_data_t *rdata, uint8_t *buf, uint16_t buflen)
{
	if (rdata == NULL || buf == NULL)
		return DNP3_FAIL;

	/* At first I was afraid, but we checked for DNP3_MAX_TRANSPORT_LEN earlier. */
	if (buflen + rdata->buflen > DNP3_BUFFER_SIZE)
		return DNP3_FAIL;
//	printf("Length in App Buff: %d \n",rdata->buflen);
	memcpy((rdata->buffer + rdata->buflen), buf, (size_t) buflen);
	if(rdata->buflen>rdata->indexOfNextResponceObjHeader)
	{
	//	printf("Next Grp: %d Var: %d \n",rdata->buffer[rdata->indexOfNextResponceObjHeader], rdata->buffer[rdata->indexOfNextResponceObjHeader+1]);
	}
	rdata->buflen += buflen;
	return DNP3_OK;
}

/* Reset a DNP3 reassembly buffer */
static void DNP3ReassemblyReset(dnp3_reassembly_data_t *rdata)
{
	rdata->buflen = 0;
		rdata->state = DNP3_REASSEMBLY_STATE__IDLE;
		rdata->last_seq = 0;
		rdata->indexOfCurrentResponceObjHeader=4;
		rdata->indexOfNextResponceObjHeader=0;
		rdata->obj_group=0;
		rdata->obj_var=0;
		rdata->start = 0;
		rdata->stop = 0;
		rdata->numberOfValues = 0;
		rdata->sizeOfRange = 0;
		rdata->sizeOfQuality = 0;
		rdata->func_code = 0;
		rdata->sizeOfData = 0;


}

/* DNP3 Transport-Layer reassembly state machine.

   Arguments:
     rdata:     DNP3 reassembly state object.
     buf:       DNP3 Transport Layer segment
     buflen:    Length of Transport Layer segment.

   Returns:
    DNP3_FAIL:     Segment was discarded.
    DNP3_OK:       Segment was queued.
 */
static int DNP3ReassembleTransport(dnp3_reassembly_data_t *rdata, uint8_t *buf, uint16_t buflen)
{
	dnp3_transport_header_t *trans_header;

	if (rdata == NULL || buf == NULL || buflen < sizeof(dnp3_transport_header_t) ||
			(buflen > DNP3_MAX_TRANSPORT_LEN))
	{
		return DNP3_FAIL;
	}

	/* Take the first byte as a transport header, cut it off of the buffer. */
	trans_header = (dnp3_transport_header_t *)buf;
	buf += sizeof(dnp3_transport_header_t);  // at this point the buffer only has application data
	buflen -= sizeof(dnp3_transport_header_t);


	/* If the previously-existing state was DONE, we need to reset it back
       to IDLE. */
	if (rdata->state == DNP3_REASSEMBLY_STATE__DONE)
		DNP3ReassemblyReset(rdata);

	switch (rdata->state)
	{
	case DNP3_REASSEMBLY_STATE__IDLE:
		/* Discard any non-first segment. */
		if ( DNP3_TRANSPORT_FIR(trans_header->control) == 0 )
			return DNP3_FAIL;

		/* Reset the buffer & queue the first segment */
		DNP3ReassemblyReset(rdata);


		DNP3QueueSegment(rdata, buf, buflen);
		rdata->last_seq = DNP3_TRANSPORT_SEQ(trans_header->control);

		if ( DNP3_TRANSPORT_FIN(trans_header->control) )
			rdata->state = DNP3_REASSEMBLY_STATE__DONE;
		else
			rdata->state = DNP3_REASSEMBLY_STATE__ASSEMBLY;

		break;

	case DNP3_REASSEMBLY_STATE__ASSEMBLY:
		/* Reset if the FIR flag is set. */
		if ( DNP3_TRANSPORT_FIR(trans_header->control) )
		{
			DNP3ReassemblyReset(rdata);

			DNP3QueueSegment(rdata, buf, buflen);
			rdata->last_seq = DNP3_TRANSPORT_SEQ(trans_header->control);
			if (DNP3_TRANSPORT_FIN(trans_header->control))
				rdata->state = DNP3_REASSEMBLY_STATE__DONE;


		}
		else
		{
			/* Same seq but FIN is set. Discard segment, BUT finish reassembly. */
			if ((DNP3_TRANSPORT_SEQ(trans_header->control) == rdata->last_seq) &&
					(DNP3_TRANSPORT_FIN(trans_header->control)))
			{

				rdata->state = DNP3_REASSEMBLY_STATE__DONE;
				return DNP3_FAIL;
			}

			/* Discard any other segments without the correct sequence. */
			if (DNP3_TRANSPORT_SEQ(trans_header->control) !=
					((rdata->last_seq + 1) % 0x40 ))
			{

				return DNP3_FAIL;
			}

			/* Otherwise, queue it up! */



			DNP3QueueSegment(rdata, buf, buflen);
			rdata->last_seq = DNP3_TRANSPORT_SEQ(trans_header->control);

			if (DNP3_TRANSPORT_FIN(trans_header->control))
				rdata->state = DNP3_REASSEMBLY_STATE__DONE;
			else
				rdata->state = DNP3_REASSEMBLY_STATE__ASSEMBLY;
		}
		break;

	case DNP3_REASSEMBLY_STATE__DONE:
		break;
	}

	/* Set the Alt Decode buffer. This must be done during preprocessing
       in order to stop the Fast Pattern matcher from using raw packet data
       to evaluate the longest content in a rule. */

	return DNP3_OK;
}

/* Check for reserved application-level function codes. */


/* Decode a DNP3 Application-layer Fragment, fill out the relevant session data
   for rule option evaluation. */
static int DNP3ProcessApplication(dnp3_session_data_t *session)
{
	dnp3_reassembly_data_t *rdata = NULL;

	if (session == NULL)
		return DNP3_FAIL;

	/* Master and Outstation use slightly different Application-layer headers.
       Only the outstation sends Internal Indications. */
	if (session->direction == DNP3_CLIENT)
	{
		dnp3_app_request_header_t *request = NULL;
		rdata = &(session->client_rdata);

		if (rdata->buflen < sizeof(dnp3_app_request_header_t))
			return DNP3_FAIL; /* TODO: Preprocessor Alert */

		request = (dnp3_app_request_header_t *)(rdata->buffer);

		session->func = request->function;
	}
	else if (session->direction == DNP3_SERVER)
	{
		dnp3_app_response_header_t *response = NULL;
		rdata = &(session->server_rdata);

		if (rdata->buflen < sizeof(dnp3_app_response_header_t))
			return DNP3_FAIL; /* TODO: Preprocessor Alert */

		response = (dnp3_app_response_header_t *)(rdata->buffer);




		session->func = response->function;
		session->indications = ntohs(response->indications);
	}



	return DNP3_OK;
}

/* Check a CRC in a single block. */
/* This code is mostly lifted from the example in the DNP3 spec. */

static inline void computeCRC(unsigned char data, uint16_t *crcAccum)
{
	*crcAccum =
			(*crcAccum >> 8) ^ crcLookUpTable[(*crcAccum ^ data) & 0xFF];
}

static int DNP3CheckCRC(unsigned char *buf, uint16_t buflen)
{
	uint16_t idx;
	uint16_t crc = 0;

	/* Compute check code for data in received block */
	for (idx = 0; idx < buflen-2; idx++)
		computeCRC(buf[idx], &crc);
	crc = ~crc; /* Invert */

	/* Check CRC at end of block */
	if (buf[idx++] == (unsigned char)crc &&
			buf[idx] == (unsigned char)(crc >> 8))
		return DNP3_OK;
	else
		return DNP3_FAIL;
}

/* Check CRCs in a Link-Layer Frame, then fill a buffer containing just the user data  */
static int DNP3CheckRemoveCRC(dnp3_config_t *config, uint8_t *pdu_start,
		uint16_t pdu_length, uint8_t *buf, uint16_t *buflen)
{
	char *cursor;
	uint16_t bytes_left;
	uint16_t curlen = 0;

	/* Check Header CRC */
	if ((config->check_crc) &&
			(DNP3CheckCRC((unsigned char*)pdu_start, sizeof(dnp3_link_header_t)+2) == DNP3_FAIL))
	{

		return DNP3_FAIL;
	}

	cursor = (char *)pdu_start + sizeof(dnp3_link_header_t) + 2;
	bytes_left = pdu_length - sizeof(dnp3_link_header_t) - 2;

	/* Process whole 16-byte chunks (plus 2-byte CRC) */
	while ( (bytes_left > (DNP3_CHUNK_SIZE + DNP3_CRC_SIZE)) &&
			(curlen + DNP3_CHUNK_SIZE < *buflen) )
	{
		if ((config->check_crc) &&
				(DNP3CheckCRC((unsigned char*)cursor, (DNP3_CHUNK_SIZE+DNP3_CRC_SIZE)) == DNP3_FAIL))
		{

			return DNP3_FAIL;
		}

		memcpy((buf + curlen), cursor, DNP3_CHUNK_SIZE);
		curlen += DNP3_CHUNK_SIZE;
		cursor += (DNP3_CHUNK_SIZE+DNP3_CRC_SIZE);
		bytes_left -= (DNP3_CHUNK_SIZE+DNP3_CRC_SIZE);
	}
	/* Process leftover chunk, under 16 bytes */
	if ( (bytes_left > DNP3_CRC_SIZE) &&
			(curlen + bytes_left < *buflen+3) )
	{
		if ((config->check_crc) && (DNP3CheckCRC((unsigned char*)cursor, bytes_left) == DNP3_FAIL))
		{

			return DNP3_FAIL;
		}

		memcpy((buf + curlen), cursor, (bytes_left - DNP3_CRC_SIZE));
		curlen += (bytes_left - DNP3_CRC_SIZE);
		cursor += bytes_left;
		bytes_left = 0;
	}

	*buflen = curlen;
	return DNP3_OK;
}

//static int DNP3CheckReservedAddrs(dnp3_link_header_t *link)
//{
//	int bad_addr = 0;
//
//	if ((link->src >= DNP3_MIN_RESERVED_ADDR) && (link->src <= DNP3_MAX_RESERVED_ADDR))
//		bad_addr = 1;
//
//	else if ((link->dest >= DNP3_MIN_RESERVED_ADDR) && (link->dest <= DNP3_MAX_RESERVED_ADDR))
//		bad_addr = 1;
//
//	if (bad_addr)
//	{
//
//		return DNP3_FAIL;
//	}
//
//	return DNP3_OK;
//}

int navigateStrtStopSpecData( dnp3_reassembly_data_t *rdata, unsigned int sizeOfOneDataPoint, unsigned int sizeOfQuality,unsigned int sizeOfRange,unsigned int sizeOfIndex,unsigned int sizeOfCtrlStatus)

{
	int done = 0;

	int modified = 0;


	int byteNumber = 0;
	int segment = 0;
	int offSet = 0;
	int byteNumberNewPktBuffer =0;
	int lengthOfCurrentPktAppData = 0;
	int startingIndex = 0;
	int startingIndexAlteredVal = 0;

	switch(rdata->obj_group){

	case 1:

		switch(rdata->obj_var)
		{
		case 1:
			sizeOfOneDataPoint = 1;
			sizeOfQuality += 0;

			memcpy(&(rdata->start),(void *)rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
			//start = ntohs(start);
			memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
			//stop = ntohs(stop);

			rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+sizeOfOneDataPoint*ceil((rdata->stop-rdata->start+1)/8.0);
			break;
		case 2:

			sizeOfOneDataPoint = 1;
			sizeOfQuality += 0;

			memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
			//start = ntohs(start);
			memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
			//stop = ntohs(stop);

			rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+sizeOfOneDataPoint*(rdata->stop-rdata->start+1);
			break;
		default:
			sizeOfOneDataPoint = 0;
			sizeOfQuality = 0;
			printf("Group or Variance not found \n");
			return -1;
		}
		break;
	case 2:
		switch(rdata->obj_var){
		case 1:
			sizeOfOneDataPoint = 1;
			sizeOfQuality += 0;
			memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
			memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
			rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+sizeOfOneDataPoint*(rdata->stop-rdata->start+1);
			break;

		case 2:
			sizeOfOneDataPoint = 7;
			sizeOfQuality += 0;
			memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
			memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
			rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+sizeOfOneDataPoint*(rdata->stop-rdata->start+1);
			break;
		case 3:
			sizeOfOneDataPoint = 3;
			sizeOfQuality += 0;
			memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
			memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
			rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+sizeOfOneDataPoint*(rdata->stop-rdata->start+1);
			break;

		default:
			sizeOfOneDataPoint = 0;
			sizeOfQuality = 0;
			printf("Group or Variance not found \n");
			return -1;
		}

			break;

		case 3:
			switch(rdata->obj_var)
			{
			case 1:
				sizeOfOneDataPoint = 1;
				sizeOfQuality = 0;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				//start = ntohs(start);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				//stop = ntohs(stop);

				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+sizeOfOneDataPoint*(rdata->stop-rdata->start+1)/4;
				break;

			case 2:
							sizeOfOneDataPoint = 1;
							sizeOfQuality = 0;

							memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
							//start = ntohs(start);
							memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
							//stop = ntohs(stop);

							rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+sizeOfOneDataPoint*(rdata->stop-rdata->start+1);
							break;

			default:
				sizeOfOneDataPoint = 0;
				sizeOfQuality = 0;
				sizeOfCtrlStatus = 0;
				printf("Group or Variance not found \n");
				done=1;
				return -1;
			}

			break;
		case 10:
		switch(rdata->obj_var)
		{
		case 1:
			sizeOfOneDataPoint = 1;
			sizeOfQuality += 0;

			memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
			memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
			rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+sizeOfOneDataPoint*(rdata->stop-rdata->start+1)/8;
			break;
		case 2:
			sizeOfOneDataPoint = 1;
			sizeOfQuality += 0;

			memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
			memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
			rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+sizeOfOneDataPoint*(rdata->stop-rdata->start+1);
			break;
		default:
			sizeOfOneDataPoint = 0;
			sizeOfQuality = 0;
			printf("Group or Variance not found \n");
			return -1;
		}

		break;

	case 12:
		switch(rdata->obj_var)
		{
		case 1:
			sizeOfOneDataPoint = 11;
			sizeOfQuality += 0;

			memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
			memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
			rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+sizeOfOneDataPoint*(rdata->stop-rdata->start+1);
			break;

		case 3:
			sizeOfOneDataPoint = 1;
			sizeOfQuality += 0;

			memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
			memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
			rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+sizeOfOneDataPoint*(rdata->stop-rdata->start+1)/8;
			break;

		default:
			sizeOfOneDataPoint = 0;
			sizeOfQuality = 0;
			printf("Group or Variance not found \n");
			return -1;
		}
		break;

		case 20:
			switch(rdata->obj_var)
			{
			case 1:
				sizeOfOneDataPoint = 4;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 2:
				sizeOfOneDataPoint = 2;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;
			case 3:
				sizeOfOneDataPoint = 4;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 4:
				sizeOfOneDataPoint = 2;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 5:
				sizeOfOneDataPoint = 4;
				sizeOfQuality += 0;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 6:
				sizeOfOneDataPoint = 2;
				sizeOfQuality += 0;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 7:
				sizeOfOneDataPoint = 4;
				sizeOfQuality += 0;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 8:
				sizeOfOneDataPoint = 2;
				sizeOfQuality += 0;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			default:
				sizeOfOneDataPoint = 0;
				sizeOfQuality = 0;
				printf("Group or Variance not found \n");
				return -1;
			}
			break;


		case 21:
			switch(rdata->obj_var)
			{
			case 1:
				sizeOfOneDataPoint = 4;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 2:
				sizeOfOneDataPoint = 2;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;
			case 3:
				sizeOfOneDataPoint = 4;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 4:
				sizeOfOneDataPoint = 2;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 5:
				sizeOfOneDataPoint = 10;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 6:
				sizeOfOneDataPoint = 8;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 7:
				sizeOfOneDataPoint = 8;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 8:
				sizeOfOneDataPoint = 8;
				sizeOfQuality += 1;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			case 9:
				sizeOfOneDataPoint = 4;
				sizeOfQuality += 0;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;
			case 10:
				sizeOfOneDataPoint = 2;
				sizeOfQuality += 0;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;
			case 11:
				sizeOfOneDataPoint = 4;
				sizeOfQuality += 0;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;
			case 12:
				sizeOfOneDataPoint = 11;
				sizeOfQuality += 0;

				memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
				memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
				rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
				break;

			default:
				sizeOfOneDataPoint = 0;
				sizeOfQuality = 0;
				printf("Group or Variance not found \n");
				return -1;
			}
			break;

			case 22:
				switch(rdata->obj_var)
				{
				case 1:
					sizeOfOneDataPoint = 4;
					sizeOfQuality += 1;

					memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;

				case 2:
					sizeOfOneDataPoint = 2;
					sizeOfQuality = 1;

					memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;
				case 3:
					sizeOfOneDataPoint = 4;
					sizeOfQuality = 1;

					memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;

				case 4:
					sizeOfOneDataPoint = 2;
					sizeOfQuality = 1;

					memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;

				case 5:
					sizeOfOneDataPoint = 10;
					sizeOfQuality = 1;

					memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;

				case 6:
					sizeOfOneDataPoint = 8;
					sizeOfQuality = 1;

					memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;

				case 7:
					sizeOfOneDataPoint = 10;
					sizeOfQuality = 1;

					memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;

				case 8:
					sizeOfOneDataPoint = 8;
					sizeOfQuality = 1;

					memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;

				default:
					sizeOfOneDataPoint = 0;
					sizeOfQuality = 0;
					printf("Group or Variance not found \n");
					return -1;
				}
				break;

				case 23:
					switch(rdata->obj_var)
							{
							case 1:
								sizeOfOneDataPoint = 4;
								sizeOfQuality = 1;

								memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
								memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
								rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
								break;

							case 2:
								sizeOfOneDataPoint = 2;
								sizeOfQuality = 1;

								memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
								memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
								rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
												break;
							case 3:
								sizeOfOneDataPoint = 4;
								sizeOfQuality = 1;

								memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
								memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
								rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
								break;

							case 4:
								sizeOfOneDataPoint = 2;
								sizeOfQuality = 1;

								memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
								memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
								rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
								break;

							case 5:
								sizeOfOneDataPoint = 10;
								sizeOfQuality = 1;

								memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
								memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
								rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
								break;

							case 6:
								sizeOfOneDataPoint = 8;
								sizeOfQuality = 1;

								memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
								memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
								rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
								break;

							case 7:
								sizeOfOneDataPoint = 10;
								sizeOfQuality = 1;

								memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
								memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
								rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
								break;

							case 8:
								sizeOfOneDataPoint = 8;
								sizeOfQuality = 1;

								memcpy(&(rdata->start),rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3,sizeOfRange/2);
								memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
								rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
								break;

							default:
								sizeOfOneDataPoint = 0;
								sizeOfQuality = 0;
								printf("Group or Variance not found \n");
								return -1;
							}
							break;
			case 30:
				switch(rdata->obj_var)
				{
				case 1:
					sizeOfOneDataPoint = 4;
					sizeOfQuality = 1;

					memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
					//start = ntohs(start);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;
				case 2:
					sizeOfOneDataPoint = 2;
					sizeOfQuality = 1;

					memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
					//start = ntohs(start);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;
				case 3:
					sizeOfOneDataPoint = 4;
					sizeOfQuality = 0;

					memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
					//start = ntohs(start);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;
				case 4:
					sizeOfOneDataPoint = 2;
					sizeOfQuality = 0;

					memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
					//start = ntohs(start);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;
				case 5:
					sizeOfOneDataPoint = 4;
					sizeOfQuality = 1;

					memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
					//start = ntohs(start);
					memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
					break;
				default:
					sizeOfOneDataPoint = 0;
					sizeOfQuality = 0;
					printf("Group or Variance not found \n");
					return -1;
				}



				break;

				case 31:
								switch(rdata->obj_var)
								{
								case 1:
									sizeOfOneDataPoint = 4;
									sizeOfQuality = 1;

									memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
									//start = ntohs(start);
									memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
									rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
									break;
								case 2:
									sizeOfOneDataPoint = 2;
									sizeOfQuality = 1;

									memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
									//start = ntohs(start);
									memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
									rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
									break;
								case 3:
									sizeOfOneDataPoint = 10;
									sizeOfQuality = 1;

									memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
									//start = ntohs(start);
									memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
									rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
									break;
								case 4:
									sizeOfOneDataPoint = 8;
									sizeOfQuality = 1;

									memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
									//start = ntohs(start);
									memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
									rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
									break;
								case 5:
									sizeOfOneDataPoint = 4;
									sizeOfQuality = 0;

									memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
									//start = ntohs(start);
									memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
									rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
									break;
								case 6:
									sizeOfOneDataPoint = 2;
									sizeOfQuality = 0;

									memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
									//start = ntohs(start);
									memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
									rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
									break;
								default:
									sizeOfOneDataPoint = 0;
									sizeOfQuality = 0;
									printf("Group or Variance not found \n");
									return -1;
								}



								break;


								case 32:
									switch(rdata->obj_var)
													{
													case 1:
														sizeOfOneDataPoint = 4;
														sizeOfQuality = 1;

														memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
														//start = ntohs(start);
														memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
														rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
														break;
													case 2:
														sizeOfOneDataPoint = 2;
														sizeOfQuality = 1;

														memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
														//start = ntohs(start);
														memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
														rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
														break;
													case 3:
														sizeOfOneDataPoint =10;
														sizeOfQuality = 1;

														memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
														//start = ntohs(start);
														memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
														rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
														break;
													case 4:
														sizeOfOneDataPoint = 8;
														sizeOfQuality = 1;

														memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
														//start = ntohs(start);
														memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
														rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
														break;


													default:
														sizeOfOneDataPoint = 0;
														sizeOfQuality = 0;
														printf("Group or Variance not found \n");
														return -1;
													}
									break;

				case 33:
					switch(rdata->obj_var)
					{
					case 1:
						sizeOfOneDataPoint = 4;
						sizeOfQuality = 1;

						memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
						//start = ntohs(start);
						memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
						rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+7+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
						break;
					case 2:
						sizeOfOneDataPoint = 2;
						sizeOfQuality = 1;

						memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
						//start = ntohs(start);
						memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
						rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
						break;
					case 3:
						sizeOfOneDataPoint =10;
						sizeOfQuality = 1;

						memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
						//start = ntohs(start);
						memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
						rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
						break;
					case 4:
						sizeOfOneDataPoint = 8;
						sizeOfQuality = 1;

						memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
						//start = ntohs(start);
						memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
						rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
						break;
					default:
						sizeOfOneDataPoint = 0;
						sizeOfQuality = 0;
						printf("Group or Variance not found \n");
						return -1;
					}
					break;

					case 40:
						switch(rdata->obj_var)
						{
						case 1:
							sizeOfOneDataPoint = 4;
							sizeOfQuality = 1;

							memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
							//start = ntohs(start);
							memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
							rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
							break;
						case 2:
							sizeOfOneDataPoint = 2;
							sizeOfQuality = 1;

							memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
							//start = ntohs(start);
							memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
							rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
							break;
						case 3:
							sizeOfOneDataPoint = 4;
							sizeOfQuality = 1;

							memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
							//start = ntohs(start);
							memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
							rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
							break;

						default:
							sizeOfOneDataPoint = 0;
							sizeOfQuality = 0;
							printf("Group or Variance not found \n");
							return -1;
						}
						break;

						case 41:
							switch(rdata->obj_var)
							{
							case 1:
								sizeOfOneDataPoint = 1;
								sizeOfQuality = 4;

								memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
								//start = ntohs(start);
								memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
								rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
								break;
							case 2:
								sizeOfOneDataPoint = 1;
								sizeOfQuality = 4;

								memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
								//start = ntohs(start);
								memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
								rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
								break;

							default:
								sizeOfOneDataPoint = 0;
								sizeOfQuality = 0;
								printf("Group or Variance not found \n");
								return -1;
							}
							break;


							case 50:
								switch(rdata->obj_var)
								{
								case 1:
									sizeOfOneDataPoint = 6;
									sizeOfQuality = 0;

									memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
									//start = ntohs(start);
									memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
									rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
									break;
								case 2:
									sizeOfOneDataPoint = 10;
									sizeOfQuality = 0;

									memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
									//start = ntohs(start);
									memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
									rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
									break;

								default:
									sizeOfOneDataPoint = 0;
									sizeOfQuality = 0;
									printf("Group or Variance not found \n");
									return -1;
								}
								break;



								case 51:
									switch(rdata->obj_var)
									{
									case 1:
										sizeOfOneDataPoint = 6;
										sizeOfQuality += 0;

										memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
										//start = ntohs(start);
										memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
										rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
										break;
									case 2:
										sizeOfOneDataPoint = 6;
										sizeOfQuality += 0;

										memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange);
										//start = ntohs(start);
										memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange);
										rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
										break;

									default:
										sizeOfOneDataPoint = 0;
										sizeOfQuality = 0;
										printf("Group or Variance not found \n");
										return -1;
									}
									break;

									case 52:
										switch(rdata->obj_var)
										{
										case 1:
											sizeOfOneDataPoint = 2;
											sizeOfQuality = 0;

											memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
											//start = ntohs(start);
											memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
											rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
											break;
										case 2:
											sizeOfOneDataPoint =2;
											sizeOfQuality = 0;

											memcpy(&(rdata->start),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange/2);
											//start = ntohs(start);
											memcpy(&(rdata->stop),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange/2),sizeOfRange/2);
											rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*(rdata->stop-rdata->start+1);
											break;

										default:
											sizeOfOneDataPoint = 0;
											sizeOfQuality = 0;
											printf("Group or Variance not found \n");
											return -1;
										}
										break;




default:
	sizeOfOneDataPoint = 0;
	sizeOfQuality = 0;
	printf("Group or Variance not found \n");
	return -1;

	break;
	}
	rdata->sizeOfData = sizeOfOneDataPoint;
	rdata->sizeOfQuality = sizeOfQuality;



	return rdata->indexOfNextResponceObjHeader;

}


int navigateQuantitySpecData( dnp3_reassembly_data_t *rdata, unsigned int sizeOfOneDataPoint, unsigned int sizeOfQuality,unsigned int sizeOfRange,unsigned int sizeOfIndex,unsigned int sizeOfCtrlStatus)

{
	int done = 0;

	int modified = 0;



	int byteNumber = 0;
	int segment = 0;
	int offSet = 0;
	int byteNumberNewPktBuffer =0;
	int lengthOfCurrentPktAppData = 0;
	int startingIndex = 0;
	int startingIndexAlteredVal = 0;




		switch(rdata->obj_group){

		case 2:
				switch(rdata->obj_var){
				case 1:
					sizeOfOneDataPoint = 1;
					sizeOfQuality += 0;
					sizeOfCtrlStatus = 0;
					memcpy(&(rdata->numberOfValues),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange);
					//start = ntohs(start);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality +sizeOfCtrlStatus)*(rdata->numberOfValues);
					break;

				case 2:
					sizeOfOneDataPoint = 7;
					sizeOfQuality += 0;
					sizeOfCtrlStatus = 0;
					memcpy(&(rdata->numberOfValues),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange);
					//start = ntohs(start);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality +sizeOfCtrlStatus)*(rdata->numberOfValues);
					break;
				case 3:
					sizeOfOneDataPoint = 3;
					sizeOfQuality += 0;
					sizeOfCtrlStatus = 0;
					memcpy(&(rdata->numberOfValues),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange);
					//start = ntohs(start);
					rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality+sizeOfCtrlStatus)*(rdata->numberOfValues);
					break;

				default:
					sizeOfOneDataPoint = 0;
					sizeOfQuality = 0;
					printf("Group or Variance not found \n");
					return -1;
				}

					break;

				case 12:
					switch(rdata->obj_var)
					{
					case 1:
						sizeOfOneDataPoint = 1;
						sizeOfQuality += 0;
						sizeOfCtrlStatus = 1;

						memcpy(&(rdata->numberOfValues),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange);
						//start = ntohs(start);

						rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality +sizeOfCtrlStatus+8)*(rdata->numberOfValues);
						break;


					default:
						sizeOfOneDataPoint = 0;
						sizeOfQuality = 0;
						printf("Group or Variance not found \n");
						return -1;
					}
					break;
					case 32:
											switch(rdata->obj_var){
											case 5:
												sizeOfOneDataPoint = 7;
												sizeOfQuality += 0;
												sizeOfCtrlStatus = 0;

												memcpy(&(rdata->numberOfValues),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange);
												//start = ntohs(start);

												rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality +sizeOfCtrlStatus)*(rdata->numberOfValues);
												break;


											default:
												sizeOfOneDataPoint = 0;
												sizeOfQuality = 0;
												printf("Group or Variance not found \n");
												return -1;

											}
											break;


		case 41:
												switch(rdata->obj_var)
												{
												case 3:
													sizeOfOneDataPoint = 4;
													sizeOfQuality += 0;
													sizeOfCtrlStatus = 1;

													memcpy(&(rdata->numberOfValues),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange);
													//start = ntohs(start);

													rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality +sizeOfCtrlStatus)*(rdata->numberOfValues);
													break;


												default:
													sizeOfOneDataPoint = 0;
													sizeOfQuality = 0;
													printf("Group or Variance not found \n");
													return -1;
												}
												break;


      case 51:
		switch(rdata->obj_var)
					{
					case 1:
						sizeOfOneDataPoint = 6;
						sizeOfQuality += 0;
						sizeOfCtrlStatus = 0;

						memcpy(&(rdata->numberOfValues),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange);


																			rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality +sizeOfCtrlStatus)*(rdata->numberOfValues);
																			break;
					case 2:
						sizeOfOneDataPoint = 6;
						sizeOfQuality += 0;
						sizeOfCtrlStatus = 0;

						memcpy(&(rdata->numberOfValues),(rdata->buffer+rdata->indexOfCurrentResponceObjHeader+3),sizeOfRange);
																			//start = ntohs(start);

																			rdata->indexOfNextResponceObjHeader = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality +sizeOfCtrlStatus)*(rdata->numberOfValues);
																			break;

					default:
						sizeOfOneDataPoint = 0;
						sizeOfQuality = 0;
						printf("Group or Variance not found \n");
						return -1;
					}
		break;

		default:
			sizeOfOneDataPoint = 0;
			sizeOfQuality = 0;
			sizeOfCtrlStatus=0;
			printf("Group or Variance not found \n");
			return -1;
			}
			rdata->sizeOfData = sizeOfOneDataPoint;
			rdata->sizeOfQuality = sizeOfQuality ;
			rdata->sizeOfIndex=sizeOfIndex;
			rdata->sizeOfCtrlStatus =sizeOfCtrlStatus;

		return rdata->indexOfNextResponceObjHeader;
}






static int modifyData(dnp3_config_t *config, dnp3_reassembly_data_t *rdata,uint16_t buflen,uint8_t * pdu_start, uint16_t pdu_length,uint8_t direction)
{

	printf("In modify \n");

	int done = 0;

	int modified = 0;

	unsigned int sizeOfOneDataPoint=0;
	unsigned int sizeOfQuality=0;
	unsigned int sizeOfRange=0;
	unsigned int sizeOfIndex = 0;
	unsigned int sizeOfCtrlStatus = 0;
	uint8_t indexSize = 0;
	uint8_t absAddress = 3;
	uint8_t quantity = 3;
	uint8_t minBufferLength = 0;
	sizeOfOneDataPoint = rdata->sizeOfData;
	sizeOfQuality = rdata->sizeOfQuality;
	sizeOfIndex = rdata->sizeOfIndex;
	sizeOfCtrlStatus = rdata->sizeOfCtrlStatus;
	sizeOfRange = rdata->sizeOfRange;
	int byteNumber = 0;
	int segment = 0;
	int offSet = 0;
	int byteNumberNewPktBuffer =0;
	int lengthOfCurrentPktAppData = 0;
	int startingIndex = 0;
	int startingIndexAlteredVal = 0;
	dnp3_app_request_header_t *request = NULL;
	dnp3_app_response_header_t *response = NULL;
	dnp3_transport_header_t * trans_header = (dnp3_transport_header_t *)((char *)pdu_start + sizeof(dnp3_link_header_t) + 2);
//	if ( DNP3_TRANSPORT_FIR(trans_header->control) == 0 )
//				return 0;


	if(rdata->buflen<=6)
			return 0;

	if(rdata->indexOfCurrentResponceObjHeader==0 || rdata->indexOfCurrentResponceObjHeader==4 || rdata->indexOfCurrentResponceObjHeader==2)
	{
		for(int i=0;i<config->numAlteredVal;i++)
		{
			(config->values_to_alter[i]).done =0;
		}

		if(direction==0){
		rdata->indexOfCurrentResponceObjHeader = 2;

		request = (dnp3_app_request_header_t *)(rdata->buffer);
		rdata->func_code = request->function;
		}
		else
		{
			rdata->indexOfCurrentResponceObjHeader = 4;
			response = (dnp3_app_response_header_t *)(rdata->buffer);
			rdata->func_code = response->function;

		}
		rdata->obj_group = rdata->buffer[rdata->indexOfCurrentResponceObjHeader];
		rdata->obj_var = rdata->buffer[rdata->indexOfCurrentResponceObjHeader+1];
		rdata-> qualifier= rdata->buffer[rdata->indexOfCurrentResponceObjHeader+2];

			rdata->start = 0;
			rdata->stop = 0;
			rdata->numberOfValues=0;

			sizeOfOneDataPoint=0;
				sizeOfQuality=0;
				sizeOfRange=0;
				sizeOfIndex = 0;
				sizeOfCtrlStatus = 0;

//	if(rdata->indexOfCurrentResponceObjHeader < rdata->buflen){
//	rdata->obj_group = rdata->buffer[rdata->indexOfCurrentResponceObjHeader];
//	rdata->obj_var = rdata->buffer[rdata->indexOfCurrentResponceObjHeader+1];
//	rdata-> qualifier= rdata->buffer[rdata->indexOfCurrentResponceObjHeader+2];
//
//	rdata->start = 0;
//	rdrdata->indexOfNextResponceObjHeaderata->stop = 0;
//	}


	indexSize = rdata->qualifier>>4;

	//Setting the Range based on the Qualifier

	switch(rdata->qualifier & 0x0F)
	{
	case 0:
		sizeOfRange = 2;
		quantity=0;
		absAddress = 0;
		minBufferLength = 7 + sizeOfRange;
		break;
	case 1:
		sizeOfRange = 4;
		quantity=0;
				absAddress = 0;
		minBufferLength = 7 + sizeOfRange;
		break;
	case 2:
		sizeOfRange = 8;
		quantity=0;
				absAddress = 0;
		minBufferLength = 7 + sizeOfRange;
		break;
	case 3:
		sizeOfRange = 1;
		absAddress= 1;
		quantity=0;
		minBufferLength = 7 + sizeOfRange;
		break;
	case 4:
		sizeOfRange = 2;
		absAddress= 1;
		quantity=0;
		minBufferLength = 7 + sizeOfRange;
		break;
	case 5:
		sizeOfRange = 4;
		absAddress= 1;
		quantity=0;
		minBufferLength = 7 + sizeOfRange;
		break;
	case 6:
		sizeOfRange = 0;
		absAddress= 3;
		quantity=3;
		minBufferLength = 7 + sizeOfRange;
		break;
	case 7:
		sizeOfRange = 1;
		quantity= 1;
		absAddress= 0;
		minBufferLength = 7 + sizeOfRange;
		break;
	case 8:
		sizeOfRange = 2;
		quantity= 1;
		absAddress= 0;
		minBufferLength = 7 + sizeOfRange;
		break;
	case 9:
		sizeOfRange = 4;
		quantity= 1;
		absAddress= 0;
		minBufferLength = 7 + sizeOfRange;
		break;

	default:
		break;
	}

	if(rdata->buflen<minBufferLength)
		return 0;


	switch(indexSize)
		{
		case 0:
			sizeOfQuality = 0;
			break;
		case 1:
			sizeOfQuality = 1;
			break;
		case 2:
			sizeOfQuality = 2;
			break;
		case 3:
			sizeOfQuality = 4;
			break;
		case 4:
			sizeOfQuality = 1;
			break;
		case 5:
			sizeOfQuality = 2;
			break;
		case 6:
			sizeOfQuality = 4;
			break;
default:
	break;
		}

	rdata->sizeOfRange = sizeOfRange;
	rdata->sizeOfQuality = sizeOfQuality;
	//put cases for each group and sub-cases for variations


	if(quantity==0 && absAddress ==0){

		navigateStrtStopSpecData( rdata, sizeOfOneDataPoint, sizeOfQuality,sizeOfRange,sizeOfIndex,sizeOfCtrlStatus);
	}

	else if(quantity==1 && absAddress ==0){
		navigateQuantitySpecData( rdata, sizeOfOneDataPoint, sizeOfQuality,sizeOfRange,sizeOfIndex,sizeOfCtrlStatus);

	}
	else
		quantity=3;


	}

	else
	{
		printf("Current ResponceHeader not set \n");


	}






	int testCount = 0;

	printf("Next Header at: %d",rdata->indexOfNextResponceObjHeader);

while(!done) //it will be done when we reach the end of buffer in rdata->server_rdata.buflen
	{
		testCount++;
		if(testCount>5)
		{
			printf( "in test count\n");

			done = 1;
		}

		if(rdata->buflen > rdata->indexOfNextResponceObjHeader)
			{
				printf("In While Next Grp: %d Var: %d \n",rdata->buffer[rdata->indexOfNextResponceObjHeader], rdata->buffer[rdata->indexOfNextResponceObjHeader+1]);
			}
		//ns3::BreakpointFallback();
		sizeOfOneDataPoint = rdata->sizeOfData;
		sizeOfQuality = rdata->sizeOfQuality;
		sizeOfRange = rdata->sizeOfRange;
		//check for the occurance of the object in the server_data and modify it
		for(int i=0;i<config->numAlteredVal;i++)
		{
//			if((config->values_to_alter[i]).done)
//			{
//				continue;
//			}
			if(quantity==0 && absAddress==0){
			if((config->values_to_alter[i]).done != 1 && (config->values_to_alter[i]).func_code == rdata->func_code && (config->values_to_alter[i]).obj_group == rdata->obj_group && (config->values_to_alter[i]).obj_var == rdata->obj_var && (config->values_to_alter[i]).identifier <=rdata->stop && (config->values_to_alter[i]).identifier >=rdata->start)
			{

				if(rdata->obj_group==1)
					byteNumber = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*((config->values_to_alter[i]).identifier -rdata->start)/8;
				else
				byteNumber = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality)*((config->values_to_alter[i]).identifier -rdata->start);
				//calculate the starting index of the data of concern in the app data of the new packet
				//if starting index is a negative with modulus less than the length of data that means the first bytes are already in the session buffer
				//should change partial data existing at the end of the pdu
				startingIndex = byteNumber-(rdata->buflen-buflen+1);    // buflen is the length of data in the current packet
				if(startingIndex<0 && abs(startingIndex)>=(sizeOfOneDataPoint+sizeOfQuality))
				{
					//you have missed the point
					continue;
				}

				else if(startingIndex<0 && abs(startingIndex)<(sizeOfOneDataPoint+sizeOfQuality))
				{

					//the data is split between two packets
					segment = 0;
					offSet =  startingIndex;
					byteNumberNewPktBuffer =  offSet +10 ;
				}

				else if(startingIndex > buflen-1)
				{
					done = 1;
					continue;

				}
				else{
					segment = (byteNumber-(rdata->buflen-buflen+1))/16;
					offSet =  (byteNumber-(rdata->buflen-buflen+1))%16;
					byteNumberNewPktBuffer = segment*18 + offSet +10;
				}

				//if the data is in segment 15 and offset over 9 then part of data will be in the next pkt

								uint8_t  tempValueToCopy[sizeOfOneDataPoint];

								float tempfloat = (config->values_to_alter[i]).floating_point_val;
								int tempInt = (config->values_to_alter[i]).integer_value;
								printf("Temp Int: %d Temp float %f\n",tempInt, tempfloat);
								memcpy(tempValueToCopy,&tempfloat,4);


								if(segment==0 && offSet <0)
								{
									startingIndexAlteredVal = abs(offSet);
								}

								else
									startingIndexAlteredVal = 0;
								//10 byte DNP3 link layer frame header, 1 byte DNP3 transport layer header, 3 byte response header and 3 bytes+sizeOfRange object header
								//copy byte by byte since the value can be between two application segments then calculate the CRC for both segments
								int twoSegments =0;
								int count = 0;
								for(int j=startingIndexAlteredVal;j<sizeOfOneDataPoint;j++){
									int temp = 1+byteNumberNewPktBuffer+sizeOfQuality+j-26;

									if((temp)==0 || ((temp)>0 && (temp)%18==0))
									{
										if(sizeOfOneDataPoint>1)
										{
										count+=2;
										twoSegments++;
										}
									}
									int dataIndexAdvance = 1;
									if(offSet ==15)
										dataIndexAdvance = 3;
									if((config->values_to_alter[i]).obj_group==1 )
									{
										int p = ((config->values_to_alter[i]).identifier%8);
										int mask = 1<<p;
										int n;
										memcpy(&n,(pdu_start+dataIndexAdvance+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),1);

										std::cout<<"Modify Binary"<<n<<endl;
										n = (n & ~mask)|((tempInt<<p)& mask);
										memcpy(tempValueToCopy,&n,4);
										memcpy((pdu_start+dataIndexAdvance+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),tempValueToCopy+j,1);

										printf("Temp Int: %d Temp float %f\n",tempInt, tempfloat);
										std::cout<<"Modify Binary To"<<n<<endl;
										modified =1;
									}
									else
									{
										if((config->values_to_alter[i]).operation==1 ){
											if(((config->values_to_alter[i]).obj_group>12 && (config->values_to_alter[i]).obj_group<40 && (config->values_to_alter[i]).obj_var>=5) ){
												memcpy((pdu_start+dataIndexAdvance+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),tempValueToCopy+j,1);
												std::cout<<"Modify Analog"<<endl;
											}

											else if ((config->values_to_alter[i]).obj_group==3)
																						{
												uint8_t byteInt = (uint8_t)tempInt;
																							memcpy(tempValueToCopy,&byteInt,1);
																							memcpy((pdu_start+dataIndexAdvance+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),tempValueToCopy+j,1);
																							std::cout<<"Modify Int out"<<endl;
																						}
											else
											{
												memcpy(tempValueToCopy,&tempInt,4);
												memcpy((pdu_start+dataIndexAdvance+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),tempValueToCopy+j,1);
												std::cout<<"Modify Int out"<<endl;
											}
										}
//										else if ((config->values_to_alter[i]).operation==2 )
//										{
//											float currentVal;
//											memcpy(&currentVal,(pdu_start+1+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),4);
//											currentVal = ntohs(currentVal)*(100+temp)/100;
//											currentVal = htons(currentVal);
//											memcpy(tempValueToCopy,&currentVal ,4);
//											memcpy((pdu_start+dataIndexAdvance+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),tempValueToCopy+j,1);
//										}
									}
									modified = 1;
									printf("Group %d Variance %d \n",rdata->obj_group,rdata->obj_var);
									count++;
								}

								(config->values_to_alter[i]).done = 1;

								{
								for(int j=0;j<=twoSegments;j++)
								{
									uint16_t idx;
									uint16_t crc = 0;
									uint8_t  temp_crc[2];
									int dataLeft = pdu_length - (segment*18 +10) -2;
									/* Compute check code for data in received block. This is for a full chunk */
									if(dataLeft>DNP3_CHUNK_SIZE){
									for (idx = 0; idx < DNP3_CHUNK_SIZE; idx++)
										computeCRC(*(pdu_start+idx+segment*18 +10), &crc);
									}
									else
									{
										for (idx = 0; idx < dataLeft; idx++)
																computeCRC(*(pdu_start+idx+segment*18 +10), &crc);
									}

									crc = ~crc; /* Invert */

									/* Check CRC at end of block */
									temp_crc[0] = (unsigned char)crc ;
									temp_crc[1] = (unsigned char)(crc >> 8);

									memcpy((pdu_start+idx+segment*18 +10),temp_crc,2);
									segment++;
								}

								}

							}
			}

			else if(quantity==1 && absAddress ==0){

				if((config->values_to_alter[i]).done != 1 && (config->values_to_alter[i]).func_code == rdata->func_code && (config->values_to_alter[i]).obj_group == rdata->obj_group && (config->values_to_alter[i]).obj_var == rdata->obj_var && (config->values_to_alter[i]).identifier <= rdata->numberOfValues  )
								{

									if(rdata->obj_group==1)
										byteNumber = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange+(sizeOfOneDataPoint+sizeOfQuality+sizeOfCtrlStatus)*((config->values_to_alter[i]).identifier)/8;
									else
									{
										int indexTemp = (int)rdata->buffer[rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange];

									if(indexTemp == (config->values_to_alter[i]).identifier)
									{
										byteNumber = rdata->indexOfCurrentResponceObjHeader+3+sizeOfRange;

									}
									else
									{
										byteNumber=rdata->buflen;
									}

									}//calculate the starting index of the data of concern in the app data of the new packet
									//if starting index is a negative with modulus less than the length of data that means the first bytes are already in the session buffer
									//should change partial data existing at the end of the pdu
									startingIndex = byteNumber-(rdata->buflen-buflen+1);    // buflen is the length of data in the current packet
									if(startingIndex<0 && abs(startingIndex)>=(sizeOfOneDataPoint+sizeOfQuality))
									{
										//you have missed the point
										continue;
									}

									else if(startingIndex<0 && abs(startingIndex)<(sizeOfOneDataPoint+sizeOfQuality+sizeOfCtrlStatus))
									{

										//the data is split between two packets
										segment = 0;
										offSet =  startingIndex;
										byteNumberNewPktBuffer =  offSet +10 ;
									}

									else if(startingIndex > buflen-1)
									{
										done = 1;
										continue;

									}
									else{
										segment = (byteNumber-(rdata->buflen-buflen+1))/16;
										offSet =  (byteNumber-(rdata->buflen-buflen+1))%16;
										byteNumberNewPktBuffer = segment*18 + offSet +10;
									}

									//if the data is in segment 15 and offset over 9 then part of data will be in the next pkt

													uint8_t  tempValueToCopy[sizeOfOneDataPoint];

													float tempFloat = (config->values_to_alter[i]).floating_point_val;
													int tempInt = (config->values_to_alter[i]).integer_value;
													memcpy(tempValueToCopy,&tempFloat,4);


													if(segment==0 && offSet <0)
													{
														startingIndexAlteredVal = abs(offSet);
													}

													else
														startingIndexAlteredVal = 0;
													//10 byte DNP3 link layer frame header, 1 byte DNP3 transport layer header, 3 byte response header and 3 bytes+sizeOfRange object header
													//copy byte by byte since the value can be between two application segments then calculate the CRC for both segments
													int twoSegments =0;
													int count = 0;
													for(int j=startingIndexAlteredVal;j<sizeOfOneDataPoint;j++){
														int temp = 1+byteNumberNewPktBuffer+sizeOfQuality+j-26;

														if((temp)==0 || ((temp)>0 && (temp)%18==0))
														{
															if(sizeOfOneDataPoint>1)
															{
															count+=2;
															twoSegments++;
															}
														}
														int dataIndexAdvance = 1;
														if(offSet ==15)
															dataIndexAdvance = 3;
														if((config->values_to_alter[i]).obj_group==1 )
														{
															int p = ((config->values_to_alter[i]).identifier%8);
															int mask = 1<<p;
															int n;
															memcpy(&n,(pdu_start+dataIndexAdvance+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),1);


															n = (n & ~mask)|((tempInt<<p)& mask);
															memcpy(tempValueToCopy,&n,4);
															memcpy((pdu_start+dataIndexAdvance+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),tempValueToCopy+j,1);


														}
														else
														{
															if((config->values_to_alter[i]).operation==1 ){
																if((config->values_to_alter[i]).obj_group>12 ){
																	memcpy((pdu_start+dataIndexAdvance+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),tempValueToCopy+j,1);
																}
																else
																{
																	memcpy(tempValueToCopy,&tempInt,4);
																	memcpy((pdu_start+dataIndexAdvance+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),tempValueToCopy+j,1);
																}
															}

//															else if ((config->values_to_alter[i]).operation==2 )
//															{
//																float currentVal;
//																memcpy(&currentVal,(pdu_start+1+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),4);
//																currentVal = ntohs(currentVal)*(100+temp)/100;
//																currentVal = htons(currentVal);
//																memcpy(tempValueToCopy,&currentVal ,4);
//																memcpy((pdu_start+dataIndexAdvance+byteNumberNewPktBuffer+sizeOfQuality+startingIndexAlteredVal+count),tempValueToCopy+startingIndexAlteredVal+j,1);
//															}
														}
														modified = 1;
														printf("Group %d Variance %d \n",rdata->obj_group,rdata->obj_var);
														count++;
													}
													(config->values_to_alter[i]).done = 1;

													for(int j=0;j<=twoSegments;j++)
													{
														uint16_t idx;
														uint16_t crc = 0;
														uint8_t  temp_crc[2];
														int dataLeft = pdu_length - (segment*18 +10) -2;
														/* Compute check code for data in received block. This is for a full chunk */
														if(dataLeft>DNP3_CHUNK_SIZE){
														for (idx = 0; idx < DNP3_CHUNK_SIZE; idx++)
															computeCRC(*(pdu_start+idx+segment*18 +10), &crc);
														}
														else
														{
															for (idx = 0; idx < dataLeft; idx++)
																					computeCRC(*(pdu_start+idx+segment*18 +10), &crc);
														}

														crc = ~crc; /* Invert */

														/* Check CRC at end of block */
														temp_crc[0] = (unsigned char)crc ;
														temp_crc[1] = (unsigned char)(crc >> 8);

														memcpy((pdu_start+idx+segment*18 +10),temp_crc,2);
														segment++;
													}

												}
											}

			}





		if((rdata->indexOfCurrentResponceObjHeader<rdata->indexOfNextResponceObjHeader && rdata->indexOfNextResponceObjHeader<rdata->buflen) && done!=1){
			rdata->indexOfCurrentResponceObjHeader = rdata->indexOfNextResponceObjHeader;

//			if(rdata->indexOfCurrentResponceObjHeader>=rdata->buflen)
//			{
//				done = 1;
//				break;
//			}


			rdata->obj_group = rdata->buffer[rdata->indexOfCurrentResponceObjHeader];
				rdata->obj_var = rdata->buffer[rdata->indexOfCurrentResponceObjHeader+1];
				 rdata->qualifier= rdata->buffer[rdata->indexOfCurrentResponceObjHeader+2];

				printf("Next Grp: %d  Var: %d \n",rdata->obj_group,rdata->obj_var);
				rdata->start = 0;
				rdata->stop = 0;
				rdata->numberOfValues = 0;

				uint8_t minBufferLength = 0;


				//Setting the Range based on the Qualifier

				switch(rdata->qualifier & 0x0F)
					{
					case 0:
						sizeOfRange = 2;
						minBufferLength = 7 + sizeOfRange;
						absAddress= 0;
																								quantity= 0;
						break;
					case 1:
						sizeOfRange = 4;
						minBufferLength = 7 + sizeOfRange;
						absAddress= 0;
																		quantity= 0;
						break;
					case 2:
						sizeOfRange = 8;
						minBufferLength = 7 + sizeOfRange;
						absAddress= 0;
												quantity= 0;
						break;
					case 3:
						sizeOfRange = 1;
						absAddress= 1;
						quantity= 0;
						minBufferLength = 7 + sizeOfRange;
						break;
					case 4:
						sizeOfRange = 2;
						absAddress= 1;
						quantity= 0;
						minBufferLength = 7 + sizeOfRange;
						break;
					case 5:
						sizeOfRange = 4;
						absAddress= 1;
						quantity= 0;
						minBufferLength = 7 + sizeOfRange;
						break;
					case 6:
						sizeOfRange = 0;
						minBufferLength = 7 + sizeOfRange;
						quantity= 3;
												absAddress= 3;
						break;
					case 7:
						sizeOfRange = 1;
						quantity= 1;
												absAddress= 0;
						minBufferLength = 7 + sizeOfRange;

						break;
					case 8:
						sizeOfRange = 2;
						quantity= 1;
												absAddress= 0;
						minBufferLength = 7 + sizeOfRange;

						break;
					case 9:
						sizeOfRange = 4;
						quantity= 1;
						absAddress= 0;
						minBufferLength = 7 + sizeOfRange;

						break;

					default:
						break;
					}



					if(rdata->buflen<minBufferLength)
						return 0;


					switch(rdata->qualifier >> 4)
						{
						case 0:
							sizeOfQuality = 0;
							break;
						case 1:
							sizeOfQuality = 1;
							break;
						case 2:
							sizeOfQuality = 2;
							break;
						case 3:
							sizeOfQuality = 4;
							break;
						case 4:
							sizeOfQuality = 1;
							break;
						case 5:
							sizeOfQuality = 2;

							break;
						case 6:
							sizeOfQuality = 4;
							break;
				default:
					break;
						}
					rdata->sizeOfRange = sizeOfRange;
					rdata->sizeOfQuality = sizeOfQuality;
				if(rdata->buflen<minBufferLength)
					return 0;


				//put cases for each group and sub-cases for variations
				if(quantity==0 && absAddress==0)	{
					navigateStrtStopSpecData( rdata, sizeOfOneDataPoint, sizeOfQuality,sizeOfRange,sizeOfIndex,sizeOfCtrlStatus);
					printf("Next Grp: %d  Var: %d \n",rdata->obj_group,rdata->obj_var);

				}

				else if(quantity==1 && absAddress ==0)
				{
					navigateQuantitySpecData( rdata, sizeOfOneDataPoint, sizeOfQuality,sizeOfRange,sizeOfIndex,sizeOfCtrlStatus);
					printf("Next Grp: %d  Var: %d \n",rdata->obj_group,rdata->obj_var);
				}



		}

		else
		{
			done =1;

		}






	}





	return modified;
}
/* Main DNP3 Reassembly function. Moved here to avoid circular dependency between
   spp_dnp3 and dnp3_reassembly. */



int DNP3FullReassembly(dnp3_config_t *config, dnp3_session_data_t *session, Ptr<const Packet> packet, uint8_t *pdu_start, uint16_t pdu_length)
{
	uint8_t buf[DNP3_TPDU_MAX];
	uint16_t buflen = sizeof(buf);
	dnp3_link_header_t *link;
	dnp3_reassembly_data_t *rdata;

	if (pdu_length < (sizeof(dnp3_link_header_t) + sizeof(dnp3_transport_header_t) + 2))
		return DNP3_FAIL;

	if ( pdu_length > DNP3_LPDU_MAX )
		// this means PAF aborted - not DNP3
		return DNP3_FAIL;

	/* Step 1: Decode header and skip to data */
	
	session->linkHeader = (dnp3_link_header_t *) pdu_start;

	if (session->linkHeader->len < DNP3_MIN_TRANSPORT_LEN)
	{

		return DNP3_FAIL;
	}


	/* XXX: NEED TO TRACK SEPARATE DNP3 SESSIONS OVER SINGLE TCP SESSION */

	//find the destination address and the source address and save the data in the session. This information should be used when modifying the data

	/* Step 2: Remove CRCs */
	if ( DNP3CheckRemoveCRC(config, pdu_start, pdu_length, buf, &buflen) == DNP3_FAIL )
	{

		return DNP3_FAIL;
	}

	/* Step 3: Queue user data in frame for Transport-Layer reassembly */
	if (session->direction == DNP3_CLIENT)
		rdata = &(session->client_rdata);
	else
		rdata = &(session->server_rdata);

	if (DNP3ReassembleTransport(rdata, buf, buflen) == DNP3_FAIL)
	{
		printf("DNP3 fail\n");
		return DNP3_FAIL;

	}



	/*
	 * write a function to take in dnp3_config, packet_data and rdata
	 */
//	if(session->direction!=DNP3_SERVER)
//	{
//		modifyData(config,rdata,buflen,packet->payload, packet->payload_size,session->direction);
//	}

	modifyData(config,rdata,buflen,pdu_start, pdu_length,session->direction);
//	{
//		if(modifyData(config,rdata,buflen,pdu_start, pdu_length,session->direction))
//		{
//			printf("Direction %s \n",session->direction==0?"ClientDirection":"ServerDirection");
//
//		}
//	}
	/* Step 4: Decode Application-Layer  */
	if (rdata->state == DNP3_REASSEMBLY_STATE__DONE)
	{
		int ret = DNP3ProcessApplication(session);

		//printf("Function %d direction %d\n",session->func,session->direction);

		/* To support multiple PDUs in UDP, we're going to call Detect()
           on each individual PDU. The AltDecode buffer was set earlier. */

			return ret;
	}

	return DNP3_OK;
}



