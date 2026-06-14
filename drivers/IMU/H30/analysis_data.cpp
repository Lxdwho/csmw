/*
* 根据不同的平台和不同的驱动包含不同的头文件
**************************************************/
//#include "xxx.h"
#include <cstddef>       
#include <cstring>       
#include <cstdint>       
#include <array>        
#include "analysis_data.h"

/*------------------------------------------------MARCOS define------------------------------------------------*/
#define PROTOCOL_FIRST_BYTE_POS 		0
#define PROTOCOL_SECOND_BYTE_POS		1

#define PROTOCOL_TID_LEN				2
#define PROTOCOL_MIN_LEN				7	/*header(2B) + tid(2B) + len(1B) + CK1(1B) + CK2(1B)*/

#define CRC_CALC_START_POS				2
#define CRC_CALC_LEN(payload_len)		((payload_len) + 3)	/*3 = tid(2B) + len(1B)*/
#define PROTOCOL_CRC_DATA_POS(payload_len)			(CRC_CALC_START_POS + CRC_CALC_LEN(payload_len))

#define PAYLOAD_POS						5

#define SINGLE_DATA_BYTES				4

/*data id define*/
#define SENSOR_TEMP_ID			(unsigned char)0x01
#define ACCEL_ID				(unsigned char)0x10
#define ANGLE_ID				(unsigned char)0x20
#define MAGNETIC_ID				(unsigned char)0x30     /*归一化值*/
#define RAW_MAGNETIC_ID			(unsigned char)0x31     /*原始值*/
#define EULER_ID				(unsigned char)0x40
#define QUATERNION_ID			(unsigned char)0x41
#define UTC_ID					(unsigned char)0x50
#define SAMPLE_TIMESTAMP_ID		(unsigned char)0x51
#define DATA_READY_TIMESTAMP_ID	(unsigned char)0x52
#define LOCATION_ID				(unsigned char)0x60		/*普通精度位置，米级*/
#define HRES_LOCATION_ID		(unsigned char)0x68		/*高精度位置，厘米级*/
#define SPEED_ID				(unsigned char)0x70
#define NAV_STATUS_ID			(unsigned char)0x80

/*length for specific data id*/
#define SENSOR_TEMP_DATA_LEN			(unsigned char)0x2
#define ACCEL_DATA_LEN					(unsigned char)12
#define ANGLE_DATA_LEN					(unsigned char)12
#define MAGNETIC_DATA_LEN				(unsigned char)12
#define MAGNETIC_RAW_DATA_LEN			(unsigned char)12
#define EULER_DATA_LEN					(unsigned char)12
#define QUATERNION_DATA_LEN				(unsigned char)16
#define UTC_DATA_LEN					(unsigned char)11
#define SAMPLE_TIMESTAMP_DATA_LEN		(unsigned char)4
#define DATA_READY_TIMESTAMP_DATA_LEN	(unsigned char)4
#define LOCATION_DATA_LEN				(unsigned char)12
#define HRES_LOCATION_DATA_LEN			(unsigned char)20
#define SPEED_DATA_LEN          		(unsigned char)12
#define NAV_STATUS_DATA_LEN				(unsigned char)1

/*factor for sensor data*/
#define NOT_MAG_DATA_FACTOR			0.000001f
#define MAG_RAW_DATA_FACTOR			0.001f
#define SENSOR_TEMP_DATA_FACTOR		0.01f

/*factor for gnss data*/
#define HRES_LONG_LAT_DATA_FACTOR	0.0000000001
#define LONG_LAT_DATA_FACTOR		0.0000001
#define ALT_DATA_FACTOR				0.001f
#define SPEED_DATA_FACTOR			0.001f

/*------------------------------------------------Variables define------------------------------------------------*/

/*------------------------------------------------Functions declare-----------------------------------------------*/


/*-------------------------------------------------------------------------------------------------------------*/
int ProtocolAnalysis::calcChecksum(uint8_t *data, uint16_t len, uint16_t *checksum) {
    uint8_t check_a = 0;
    uint8_t check_b = 0;

    if (nullptr == data || 0 == len || nullptr == checksum) {
        return para_err;
    }

    for (uint16_t i = 0; i < len; ++i) {
        check_a += data[i];
        check_b += check_a;
    }

    *checksum = static_cast<uint16_t>((check_b << 8) | check_a);

    return analysis_ok;
}

int ProtocolAnalysis::checkDataLenById(payload_data_t header, uint8_t *data, protocol_info_t *info)
{
	unsigned char ret = 0xff;

	if(NULL == data || (unsigned char)0 == header.data_len || NULL == info)
	{
		return para_err;
	}

	switch(header.data_id)
	{
		case SENSOR_TEMP_ID:
		{
			if(SENSOR_TEMP_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->sensor_temp = getSignedShort(data) * SENSOR_TEMP_DATA_FACTOR;
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case ACCEL_ID:
		{
			if(ACCEL_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->accel.x = getSignedInt(data) * NOT_MAG_DATA_FACTOR;
				info->accel.y = getSignedInt(data + SINGLE_DATA_BYTES) * NOT_MAG_DATA_FACTOR;
				info->accel.z = getSignedInt(data + SINGLE_DATA_BYTES * 2) * NOT_MAG_DATA_FACTOR;
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case ANGLE_ID:
		{
			if(ANGLE_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->angle_rate.x = getSignedInt(data) * NOT_MAG_DATA_FACTOR;
				info->angle_rate.y = getSignedInt(data + SINGLE_DATA_BYTES) * NOT_MAG_DATA_FACTOR;
				info->angle_rate.z = getSignedInt(data + ANGLE_DATA_LEN - SINGLE_DATA_BYTES) * NOT_MAG_DATA_FACTOR;
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case MAGNETIC_ID:
		{
			if(MAGNETIC_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->norm_mag.x = getSignedInt(data) * NOT_MAG_DATA_FACTOR;
				info->norm_mag.y = getSignedInt(data + SINGLE_DATA_BYTES) * NOT_MAG_DATA_FACTOR;
				info->norm_mag.z = getSignedInt(data + SINGLE_DATA_BYTES * 2) * NOT_MAG_DATA_FACTOR;
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case RAW_MAGNETIC_ID:
		{
			if(MAGNETIC_RAW_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->raw_mag.x = getSignedInt(data) * MAG_RAW_DATA_FACTOR;
				info->raw_mag.y = getSignedInt(data + SINGLE_DATA_BYTES) * MAG_RAW_DATA_FACTOR;
				info->raw_mag.z = getSignedInt(data + SINGLE_DATA_BYTES * 2) * MAG_RAW_DATA_FACTOR;
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case EULER_ID:
		{
			if(EULER_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->attitude.pitch = getSignedInt(data) * NOT_MAG_DATA_FACTOR;
				info->attitude.roll = getSignedInt(data + SINGLE_DATA_BYTES) * NOT_MAG_DATA_FACTOR;
				info->attitude.yaw = getSignedInt(data + SINGLE_DATA_BYTES * 2) * NOT_MAG_DATA_FACTOR;
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case QUATERNION_ID:
		{
			if(QUATERNION_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->attitude.quaternion_data0 = getSignedInt(data) * NOT_MAG_DATA_FACTOR;
				info->attitude.quaternion_data1 = getSignedInt(data + SINGLE_DATA_BYTES) * NOT_MAG_DATA_FACTOR;
				info->attitude.quaternion_data2 = getSignedInt(data + SINGLE_DATA_BYTES * 2) * NOT_MAG_DATA_FACTOR;
				info->attitude.quaternion_data3 = getSignedInt(data + SINGLE_DATA_BYTES * 3) * NOT_MAG_DATA_FACTOR;
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case LOCATION_ID:
		{
			if(LOCATION_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->location.latitude  = getSignedInt(data) * LONG_LAT_DATA_FACTOR;
				info->location.longtidue = getSignedInt(data + SINGLE_DATA_BYTES) * LONG_LAT_DATA_FACTOR;
				info->location.altidue 	 = getSignedInt(data + SINGLE_DATA_BYTES * 2)  * ALT_DATA_FACTOR;
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case HRES_LOCATION_ID:
		{
			if(HRES_LOCATION_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->location.latitude  = *((long long int *)data) * HRES_LONG_LAT_DATA_FACTOR;
				info->location.longtidue = *((long long int *)(data + SINGLE_DATA_BYTES * 2)) * HRES_LONG_LAT_DATA_FACTOR;
				info->location.altidue 	 = getSignedInt(data + HRES_LOCATION_DATA_LEN - SINGLE_DATA_BYTES)  * ALT_DATA_FACTOR;
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case SPEED_ID:
		{
			if(SPEED_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->vel.vel_n = getSignedInt(data) * SPEED_DATA_FACTOR;
				info->vel.vel_e = getSignedInt(data + SINGLE_DATA_BYTES) * SPEED_DATA_FACTOR;
				info->vel.vel_d = getSignedInt(data + SINGLE_DATA_BYTES * 2) * SPEED_DATA_FACTOR;
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case UTC_ID:
		{
			if(UTC_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				memcpy((unsigned char *)&info->utc.msecond, data, sizeof(utc_time_t));
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case NAV_STATUS_ID:
		{
			if(NAV_STATUS_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->status = *((nav_status_t *)data);
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case SAMPLE_TIMESTAMP_ID:
		{
			if(SAMPLE_TIMESTAMP_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->sample_timestamp = *((unsigned int *)data);
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		case DATA_READY_TIMESTAMP_ID:
		{
			if(DATA_READY_TIMESTAMP_DATA_LEN == header.data_len)
			{
				ret = analysis_ok;
				info->data_ready_timestamp = *((unsigned int *)data);
			}
			else
			{
				ret = data_len_err;
			}
		}
		break;

		default:
		break;
	}

	return ret;
}

/*--------------------------------------------------------------------------------------------------------------
* 输出协议为：header1(0x59) + header2(0x53) + tid(2B) + payload_len(1B) + payload_data(Nbytes) + ck1(1B) + ck2(1B)
* crc校验从TID开始到payload data的最后一个字节
*/
int ProtocolAnalysis::analysisData(uint8_t *data, int16_t len, protocol_info_t *info)
{
	unsigned short payload_len = 0;
	unsigned short check_sum = 0;
	unsigned short pos = 0;
	int ret = analysis_done;

	output_data_header_t *header = NULL;
	payload_data_t *payload = NULL;

	if(NULL == data || 0 >= len)
	{
		return para_err;
	}

	if(len < PROTOCOL_MIN_LEN)
	{
        return data_len_err;
	}

	/*judge protocol header*/
	if(PROTOCOL_FIRST_BYTE == data[PROTOCOL_FIRST_BYTE_POS] && \
		PROTOCOL_SECOND_BYTE == data[PROTOCOL_SECOND_BYTE_POS])
	{
		/*further check*/
		output_data_header_t *header = reinterpret_cast<output_data_header_t *>(data);
		payload_len = header->len;

		if(payload_len + PROTOCOL_MIN_LEN > len)
		{
			return 	data_len_err;
		}

		/*checksum*/
		calcChecksum(data + CRC_CALC_START_POS, CRC_CALC_LEN(payload_len), &check_sum);
		if(check_sum != *reinterpret_cast<uint16_t *>(data + PROTOCOL_CRC_DATA_POS(payload_len)))
		{
			return crc_err;
		}

		/*analysis payload data*/
		pos = PAYLOAD_POS;

		while(payload_len > 0)
		{
			payload_data_t *payload = reinterpret_cast<payload_data_t *>(data + pos);
        	ret = checkDataLenById(*payload, data + pos + sizeof(payload_data_t), info);
			if(analysis_ok == ret)
			{
				pos += payload->data_len + sizeof(payload_data_t);
				payload_len -= payload->data_len + sizeof(payload_data_t);
			}
			else
			{
				pos++;
				payload_len--;
			}
		}

		return analysis_ok;
	}
	else
	{
		return analysis_done;
	}
	return 0;
}


int16_t ProtocolAnalysis::getSignedShort(uint8_t *data) {
    int16_t temp = 0;
    temp = static_cast<int16_t>((data[1] << 8) | data[0]);
    return temp;
}

int32_t ProtocolAnalysis::getSignedInt(uint8_t *data) {
    int32_t temp = 0;
    temp = static_cast<int32_t>((static_cast<int32_t>(data[3]) << 24) |
                                 (static_cast<int32_t>(data[2]) << 16) |
                                 (static_cast<int32_t>(data[1]) << 8) |
                                 static_cast<int32_t>(data[0]));
    return temp;
}
