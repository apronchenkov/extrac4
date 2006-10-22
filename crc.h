#ifndef __crc_h__
#define __crc_h__

#ifndef _WIN32
#include <sys/types.h>
#else
typedef unsigned long u_int32_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif
							/** ��������� �������� ������� ��� ���ޣ�� CRC32.
							 */
		void crc_gen();

							/** ������� ����������� �������� CRC32 ��� ������.
								@param crc ������� �������� crc
								@param string ������� ������
								@return ����� ����������� �����
							*/
		u_int32_t crc_calc_string(u_int32_t crc, const char *string);

							/** ������� ����������� �������� CRC ��� ������� ����.
								@param ������� �������� crc
								@param array ������� ������ ������
								@param size ������ ������� ������
								@return ����� ����������� �����
							*/
		u_int32_t crc_calc_array(u_int32_t crc, const char *array, size_t size);

#ifdef __cplusplus
}
#endif

		
#endif //__crc_h__
