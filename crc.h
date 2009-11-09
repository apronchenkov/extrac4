#ifndef __crc_h__
#define __crc_h__

#ifndef _WIN32
#include <sys/types.h>
#else
typedef unsigned long u_int32_t;
#endif

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
	/**
	 * Процедура строящая таблицу для расчёта CRC32.
	 */
	void crc_gen();

	/** Функция вычисляющая значение CRC32 для строки.
	 * @param crc текущее значение crc
	 * @param string входная строка
	 * @return общая контрольная сумма
	 */
	u_int32_t crc_calc_string(u_int32_t crc, const char *string);

	/**
	 * Функция вычисляющая значение CRC для массива байт.
	 * @param текущее значение crc
	 * @param array входной массив байтов
	 * @param size длинна массива байтов
	 * @return общая контрольная сумма
	 */
	u_int32_t crc_calc_array(u_int32_t crc, const char *array, size_t size);

#ifdef __cplusplus
}
#endif

#endif /*_crc_h__*/
