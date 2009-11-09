/*
 *  extract.c by Phrack Staff and sirsyko
 *
 *  Copyright (c) 1997 - 2006 Phrack Magazine
 *
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  TODO:
 *  - find all errors;
 *  - simplify source code and comment it;
 *  - find all errors;
 *  - check CRC32 algorithm, especially for Little/Big Endian compatible;
 *  - check BASE64 algorithm.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "base64.h"
#include "crc.h"

#ifdef _WIN32
#include <direct.h>
#define mkdir(dirname, mode) _mkdir(dirname)
#endif

#define VERSION_STR          ("20060909 revision d")

/** Открывающийся тэг. */
#define BEGIN_TAG            ("<++>")
#define BEGIN_TAG_LEN        (sizeof(BEGIN_TAG) - 1)

/** Закрывающийся тэг. */
#define END_TAG              ("<-->")
#define END_TAG_LEN          (sizeof(END_TAG) - 1)

/** Расширенная версия открывающегося тега. */
#define BEGIN2_TAG            ("//<++>")
#define BEGIN2_TAG_LEN        (sizeof(BEGIN2_TAG) - 1)

/** Расширенная версия закрывающегося тега. */
#define END2_TAG              ("//<-->")
#define END2_TAG_LEN          (sizeof(END2_TAG) - 1)

/** Максимальная длинна строки. */
/** Крайне желательно, чтобы эта величина делилась на 4 (связано с преобразованием base64). */
#define MAX_LINESIZE         (2048)

/** Максимальный размер для строки путь/имя. */
#define MAX_PATHNAME         (1024)


/** Имя программы. */
const char *prog_name;

/** Флаги выполнения. */
const int QUIET            = 1;
int flags;

/** Статистика: количество найденных записей. */
int stat_found;

/** Статистика: количество успешно распакованных записей. */
int stat_extracted;

/** ПутьИмя входного файла. */
char *in_pathname;

/** ПутьИмя выходного файла. */
char out_pathname[ MAX_PATHNAME + 1 ];

/** Формат текущей записи. */
enum{TXT, B64} format;


/** Флаг проверки контрольной суммы .*/
bool crc_check_flag;

/** Исходная контрольная сумма. */
u_int32_t crc_old_value;
/** Новая контрольная сумма. */
u_int32_t crc_value;


/** Множество всех символов. */
#define isall(c) (true)
/** Множество символов -- признаков конца строки. */
#define iseol(c) ('\0' == (c) || '\n' == (c) || '\r' == (c))
/** Множество пробельных символов. */
#define isspace(c) (' ' == (c) || '\t' == (c))
/** Множество специальных символов. */
#define isspecial(c) ('\\' == (c) || '/' == (c))
/** Расширенное множество (простых) символов. */
#define isextra(c) (!iseol(c))
/** Множество простых символов. */
#define ischar(c) (isextra(c) && !isspace(c) && !isspecial(c))


/** Функция выполняет разбора тега.
 * На основе тэга устанавливаются глобальные переменные: out_pathname, ...
 * @param head строка с параметрами открывающегося тэга
 * @return true -- разбор успешен; false -- заголовок содержит ошибку.
 */
bool parse_tag(char * head) {
	char * b = head;
	char * n = NULL;
	char * nmax = NULL;

	/* проходим ведущие пробелы */
	while( isspace(*b) ) ++b;
	/* должно присутствовать путь_имя файла */
	if( iseol(*b) ) {
		if( !(flags & QUIET ) )
			fprintf(stderr, "%s", "Incorrect tag");
		else
			fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect tag.");
		return false;
	}

	/* готовимся к разбору имени */
	n = out_pathname;
	nmax = out_pathname + sizeof(out_pathname) - 1;

	/* разбор имени */
	while( true ) {
		const char * t = NULL;

		if( '.' == *b || ('\\' == *b && '.' == b[1]) ) {
			if( !(flags & QUIET ) )
				fprintf(stderr, "%s", "Incorrect pathname field");
			else
				fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect pathname field.");
			return false;
		}

		t = n;
		while( true ) {
			while( ischar(*b) && n + 1 != nmax )
				*n++ = *b++;

			if( iseol(*b) || isspace(*b) )
				break;

			if( '/' == *b )
				break;

			if( '\\' == *b ) {
				++b;
				if( isspecial(b[1]) )
					break;

				if( iseol(*b) ) {
					if( !(flags & QUIET ) )
						fprintf(stderr, "%s", "Incorrect pathname field");
					else
						fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect pathname field.");
					return false;
				}
			}

			if( n + 1 == nmax ) {
				if( !(flags & QUIET ) )
					fprintf(stderr, "%s", "Too longpath pathname field.");
				else
					fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect pathname field.");
				return false;
			}

			*n++ = *b++;
		}

		if( t == n ) {
			if( !(flags & QUIET ) )
				fprintf(stderr, "%s", "Incorrect pathname field");
			else
				fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect pathname field.");
			return false;
		}

		if( iseol(*b) || isspace(*b) )
			break;

		if( n + 1 == nmax ) {
			if( !(flags & QUIET ) )
				fprintf(stderr, "%s", "Too longpath pathname field.");
			else
				fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect pathname field.");
			return false;
		}

		*n++ = '/';
		++b;
	}

	*n = '\0';

	/* пробел перед опциями */
	while( isspace(*b) ) ++b;

	while( !iseol(*b) ) {

		if( '!' != *b++ ) {
			if( !(flags & QUIET ) )
				fprintf(stderr, "%s", "There must be '!' befor each option");
			else
				fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect option.");
			return false;
		}

		if( 0 == strncmp("text", b, 4) ) {
			format = TXT;
			b += 4;

		} else if( 0 == strncmp("base64", b, 6) ) {
			format = B64;
			b += 6;

		} else if( 0 == strncmp("comment", b, 7) ) {
			break;

		} else { /* CRC32 */
			errno = 0;
			crc_old_value = strtoul(b, &b, 16);

			if( 0 != errno ) {
				if( !(flags & QUIET ) )
					fprintf(stderr, "%s", "Option CRC32 contain incorrect value");
				else
					fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect option.");
				return false;
			}

			crc_check_flag = true;
		}

		while( isspace(*b) ) ++b;
	}

	return true;
}

/**
 * Функция распаковки txt-файлов.
 */
bool unpack_txt(FILE *out, char *line) {
	return EOF != fputs(line, out);
}

/**
 * Функция распаковки b64-файлов.
 */
bool unpack_b64(FILE *out, char *line) {
	char outbuf[ MAX_LINESIZE ];
	size_t outlen = sizeof(outbuf);
	size_t line_len = strlen(line);

	while( iseol( line[line_len - 1] ) ) --line_len;

	if( false == base64_decode(line, line_len, outbuf, &outlen) ) {
		if( !(flags & QUIET ) )
			fprintf(stderr, "%s", ". Incorrect base64 codedata");
		else
			fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect base64 code.");
		return false;
	}

	return 1 == fwrite(outbuf, outlen, 1, out);
}

/**
 * Процедура чтения входного файла.
 * Процедура разбирает данные входного файла, выделяет тэги, считывает данные.
 * @param in поток входного файла.
 */
void parse_file(FILE *in) {
	char b_tmp[ MAX_LINESIZE ];

	while( 1 ) {
		char * b = NULL;

		/* поиск тега начала блока */
		while( b == NULL && fgets(b_tmp, sizeof(b_tmp), in) ) {
			if( 0 == strncmp(b_tmp, BEGIN_TAG, BEGIN_TAG_LEN) )
				b = b_tmp + BEGIN_TAG_LEN;
			if( 0 == strncmp(b_tmp, BEGIN2_TAG, BEGIN2_TAG_LEN) )
				b = b_tmp + BEGIN2_TAG_LEN;
		}

		/* если произошла ошибка или наступил конец файла, завершаем разбор */
		if( b == NULL )
			break;

		/* увеличиваем счётчик найденных тегов */
		++stat_found;

		/* установка параметров по умолчанию */
		format = TXT;
		crc_check_flag = false;
		crc_value = 0;
		/* приём позволяющий измежать использования goto */
		/* разбор заголовка */
		while( parse_tag(b) ) {
			char *bp;
			FILE *out;

			if( !(flags & QUIET) )
				fprintf(stderr, "  Extracting '%s'..", out_pathname);
			/* создаём ветку каталогов */
			for(bp = out_pathname; NULL != (bp = strchr(bp, '/')); ++bp) {
				*bp = '\0';
				if( -1 == mkdir(out_pathname, 0755) && EEXIST != errno )
					break;
				*bp = '/';
			}

			/* если не удалось создать ветку каталогов, выводим ошибку и пропускаем данный тег */
			if( NULL != bp ) {
				if( !(flags & QUIET ) )
					fprintf(stderr, "%s '%s'", ". Can't create directory", out_pathname);
				else
					fprintf(stderr, "%s: %s '%s'.\n", in_pathname, "Can't create directory", out_pathname);
				break;
			}
			/* если не удалось создать/открыть файл, выводим ошибку и пропускаем данный тег */
			if( !(out = fopen(out_pathname, "wb")) ) {
				if( !(flags & QUIET) )
					fprintf(stderr, "%s", ". Can't create/open file");
				else
					fprintf(stderr, "%s: %s '%s'.\n", in_pathname, "Can't create/open file", out_pathname);
				break;
			}
			/* распаковываем содержимое файла */
			while( fgets(b_tmp, sizeof(b_tmp), in) ) {
				if( 0 == strncmp(b_tmp, END_TAG, END_TAG_LEN) )
					break;
				if( 0 == strncmp(b_tmp, END2_TAG, END2_TAG_LEN) )
					break;

				if( TXT == format ) {
					if( !unpack_txt(out, b_tmp) )
						break;
				} else if( B64 == format ) {
					if( !unpack_b64(out, b_tmp) )
						break;
				}
				if( crc_check_flag )
					crc_value = crc_calc_string(crc_value, b_tmp);

			}
			{
				/* сохраняем флаг ошибки. */
				int rec = ferror(out);

				fclose(out);
				/* проверяем флаг ошибки. */
				if( rec ) {
					if( !(flags & QUIET) )
						fprintf(stderr, "%s", ". write error occurred");
					else
						fprintf(stderr, "%s: %s '%s'.\n", in_pathname, "Write error occurred during extracting", out_pathname);
					break;
				}
			}

			++stat_extracted;

			break;
		}

		/* пропускаем всю оставшуюся информацию до завершающего тэга */
		/* (необходимо в случае ошибки) */
		while( 0 != strncmp(b_tmp, END_TAG, END_TAG_LEN) && 0 != strncmp(b_tmp, END2_TAG, END2_TAG_LEN) ) {
			if( NULL == fgets(b_tmp, sizeof(b_tmp), in) )
				break;
		}

		/* если произошла ошибка ввода */
		if( ferror(in) )
			break;

		/* проверяем контрольную сумму */
		if( crc_check_flag ) {

			if( crc_old_value == crc_value ) {
				if( !(flags & QUIET) )
					fprintf(stderr, "%s (%08lx)", ". CRC32 verified", crc_value);
			} else {
				if( !(flags & QUIET) )
					fprintf(stderr, "%s (%08lx != %08lx)", ". CRC32 failed", crc_old_value, crc_value);
				else
					fprintf(stderr, "%s: %s: %s (%08lx != %08lx).\n", in_pathname, out_pathname, "CRC32 faild", crc_old_value, crc_value);
			}
		}
		/* завершаем работу с текущим тэгом. */
		if( !(flags & QUIET) )
			fprintf(stderr, ".\n");
	}
}

/**
 * Процедура печати инструкции.
 * Процедура завершает работу программы.
 */
static void usage() {
	fprintf(stderr, "%s%s%s\n", "Usage: ", prog_name, "[-qv] file1 [file2 ... filen]");
	exit(EXIT_FAILURE);
}

/**
 * Процедура печати версии программы.
 * Процедура завершает работу программы.
 */
static void version() {
	fprintf(stderr, "%s%s\n", "Extract : ", VERSION_STR);
	exit(EXIT_FAILURE);
}

int optind;
/**
 * Процедура разбора параметров командной строки.
 * На основе разбора устанавливается глобальная переменная flags.
 * @param argc количество параметров
 * @param argv список параметров
 */
static void parse_args(int argc, char **argv) {
	if( 1 == argc )
		usage();

	if( 0 == strcmp("-q", argv[1]) ) {
		flags |= QUIET;
		optind = 2;
	} else if( 0 == strcmp("-v", argv[1]) ) {
		version();
	} else
		optind = 1;
}

/** 
 * Начальная функция программы.
 */
int main(int argc, char **argv) {
	int i;

	/* запоминаем имя программы */
	prog_name = argv[0];

	parse_args(argc, argv);

	crc_gen();

	/* последовательно просматриваем аргументы командной строки */
	for(i = optind; i < argc; ++i) {
		FILE *in;

		in_pathname = argv[i];
		/* открываем файл */
		if( 0 == strcmp(in_pathname, "-") ) {
			in = stdin;
			in_pathname = "stdin";

		} else if( !(in = fopen(in_pathname, "rb")) ) {
			fprintf(stderr, "Can't open input file '%s'.\n", in_pathname);
			continue;
		}

		if( !(flags & QUIET) )
			fprintf(stderr, "Scanning '%s'...\n", in_pathname);

		parse_file(in);

		if( ferror(in) )
			fprintf(stderr, "%s: Read error occurred during parse input file.\n", in_pathname);

		fclose(in);
	}

	/* вывод статистики */
	fprintf(stderr, "There are %d record(s), extracted %d record(s).\n", stat_found, stat_extracted);

	/* обработка статистики */
	if( stat_extracted == stat_found )
		return EXIT_SUCCESS;

	return EXIT_FAILURE;
}
/* EOF */
