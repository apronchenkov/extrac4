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

const char version_str[]   = "20060909 revision d";

					/// ������������� ���.
const char begin_tag[]     = "<++>";
const size_t begin_tag_len = sizeof(begin_tag) - 1;
					/// ������������� ���.
const char end_tag[]       = "<-->";
const size_t end_tag_len   = sizeof(end_tag) - 1;
					/// ������������ ������ ������.
					/// ������ ����������, ����� ��� �������� �������� �� 4 (������� � ��������������� base64).
const size_t max_linesize  = 2048;
					/// ������������ ������ ��� ������ ����/���.
const size_t max_pathname  = 1024;

					/// ��� ���������.
const char *prog_name;

					/// ����� ����������.
const int QUIET            = 1;
int flags;
					/// ����������: ���������� ��������� �������.
int stat_found;
					/// ����������: ���������� ������� ������������� �������.
int stat_extracted;

					/// ������� �������� �����.
char *in_pathname;
					/// ������� ��������� �����.
char out_pathname[ max_pathname + 1 ];
					/// ������ ������� ������.
enum{TXT, B64} format;

					/// ���� �������� ����������� �����
bool crc_check_flag;
					/// �������� ����������� �����.
u_int32_t crc_old_value;
					/// ����� ����������� �����.
u_int32_t crc_value;

					/// ��������� ���� ��������.
inline bool isall(char c) {return true;}
					/// ��������� �������� -- ��������� ����� ������.
inline bool iseol(char c) {return ('\0' == c || '\n' == c || '\r' == c);}
					/// ��������� ���������� ��������.
inline bool isspace(char c) {return (' ' == c || '\t' == c);}
					/// ��������� ����������� ��������.
inline bool isspecial(char c) {return ('\\' == c || '/' == c);}
					/// ����������� ��������� (�������) ��������.
inline bool isextra(char c) {return !iseol(c);}
					/// ��������� ������� ��������.
inline bool ischar(char c) {return (isextra(c) && !isspace(c) && !isspecial(c));}

					/** ������� ��������� ������� ����.
						�� ������ ���� ��������������� ���������� ����������: out_pathname, ...
						@param head ������ ���������� ��������� ����
						@return true -- ������ �������; false -- ��������� �������� ������.
					*/
bool parse_tag(char *head) {
							// �������� ��� ����
		char *b = head + begin_tag_len;
							// �������� ������� �������
		while( isspace(*b) ) ++b;
							// ������ �������������� ����_��� �����
		if( iseol(*b) ) {
				if( !(flags & QUIET ) ) 
						fprintf(stderr, "%s", "Incorrect tag");
				else
						fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect tag.");
				return false;
		}
							// ��������� � ������� �����
		char *n = out_pathname;
		const char *nmax = out_pathname + sizeof(out_pathname) - 1;
							// ������ �����
		while( true ) {
				if( '.' == *b || 
					('\\' == *b && '.' == b[1]) ) {
						if( !(flags & QUIET ) ) 
								fprintf(stderr, "%s", "Incorrect pathname field");
						else
								fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect pathname field.");
						return false;
				}
				
				const char *t = n;
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
		
							// ������ ����� �������
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
						
				} else {// CRC32
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
					/** ������� ���������� txt-������. 
					 */
bool unpack_txt(FILE *out, char *line) {
		return EOF != fputs(line, out);
}

					/** ������� ���������� b64-������. 
					 */
bool unpack_b64(FILE *out, char *line) {
		size_t line_len = strlen(line);
		while( iseol( line[line_len - 1] ) ) --line_len;
		
		char outbuf[ max_linesize ];
		size_t outlen = sizeof(outbuf);

		if( false == base64_decode(line, line_len, outbuf, &outlen) ) {
				if( !(flags & QUIET ) ) 
						fprintf(stderr, "%s", ". Incorrect base64 codedata");
				else
						fprintf(stderr, "%s: %s\n", in_pathname, "Incorrect base64 code.");
				return false;
		}
		
		return 1 == fwrite(outbuf, outlen, 1, out);
}

					/** ��������� ������ �������� �����. 
						��������� ��������� ������ �������� �����, �������� ����, ��������� ������.
						@param in ����� �������� �����.
					*/
void parse_file(FILE *in) {

		while( 1 ) {
				char b_tmp[ max_linesize ];

									// ����� ���� ������ �����
				while( NULL != fgets(b_tmp, max_linesize - 1, in) &&
					   0 != strncmp(b_tmp, begin_tag, begin_tag_len) );
									// ���� ��������� ������ ��� �������� ����� �����, ��������� ������
				if( ferror(in) || feof(in) )
						break;
									// ����������� �ޣ���� ��������� �����
				++stat_found;
				
									// ��������� ���������� �� ���������
				format = TXT;
				crc_check_flag = false;
				crc_value = 0;
									// ��ɣ� ����������� �������� ������������� goto
									// ������ ���������
				while( parse_tag(b_tmp) ) {
						char *bp;
						FILE *out;
						
						if( !(flags & QUIET) )
								fprintf(stderr, "  Extracting '%s'..", out_pathname);
											// ������� ����� ���������
						for(bp = out_pathname; NULL != (bp = strchr(bp, '/')); ++bp) {
								*bp = '\0';
								if( -1 == mkdir(out_pathname, 0755) && EEXIST != errno )
										break;
								*bp = '/';
						}
											// ���� �� ������� ������� ����� ���������, ������� ������ � ���������� ������ ���
						if( NULL != bp ) {
								if( !(flags & QUIET ) )
										fprintf(stderr, "%s '%s'", ". Can't create directory", out_pathname);
								else
										fprintf(stderr, "%s: %s '%s'.\n", in_pathname, "Can't create directory", out_pathname);
								break;
						}
											// ���� �� ������� �������/������� ����, ������� ������ � ���������� ������ ���
						if( !(out = fopen(out_pathname, "wb")) ) {
								if( !(flags & QUIET) )
										fprintf(stderr, "%s", ". Can't create/open file");
								else
										fprintf(stderr, "%s: %s '%s'.\n", in_pathname, "Can't create/open file", out_pathname);
								break;
						}
											// ������������� ���������� �����
						while( fgets(b_tmp, sizeof(b_tmp), in) && 0 != strncmp (b_tmp, end_tag, end_tag_len) ) {
								if( TXT == format ) {
										if( !unpack_txt(out, b_tmp) ) break;
								} else if( B64 == format ) {
										if( !unpack_b64(out, b_tmp) ) break;
								}
								if( crc_check_flag )
										crc_value = crc_calc_string(crc_value, b_tmp);
								
						}
											// ��������� ���� ������.
						int rec = ferror(out);
						
						fclose(out);
											// ��������� ���� ������.
						if( rec ) {
								if( !(flags & QUIET) ) 
										fprintf(stderr, "%s", ". write error occurred");
								else
										fprintf(stderr, "%s: %s '%s'.\n", in_pathname, "Write error occurred during extracting", out_pathname);
								break;
						}
						
						++stat_extracted;

						break;
				}
									// ���������� ��� ���������� ���������� �� ������������ ���� 
									// (���������� � ������ ������)
				while( 0 != strncmp(b_tmp, end_tag, end_tag_len) && fgets(b_tmp, sizeof(b_tmp), in) );

									// ���� ��������� ������ �����
				if( ferror(in) ) 
						break;

									// ��������� ����������� �����
									// ��������� ����������� �����
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
									// ��������� ������ � ������� �����.
				if( !(flags & QUIET) )
						fprintf(stderr, ".\n");
		}
}

					/** ��������� ������ ����������.
						��������� ��������� ������ ���������.
					 */
static void usage() {
		fprintf(stderr, "%s%s%s\n", "Usage: ", prog_name, "[-qv] file1 [file2 ... filen]");
		exit(EXIT_FAILURE);
}

					/** ��������� ������ ������ ���������. 
						��������� ��������� ������ ���������.
					 */
static void version() {
		fprintf(stderr, "%s%s\n", "Extract : ", version_str);
		exit(EXIT_FAILURE);
}

int optind;
					/** ��������� ������� ���������� ��������� ������.
						�� ������ ������� ��������������� ���������� ���������� flags.
						@param argc ���������� ����������
						@param argv ������ ����������
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

					/** ��������� ������� ���������.
					 */
int main(int argc, char **argv) {
							// ���������� ��� ���������
		prog_name = argv[0];

		parse_args(argc, argv);

		crc_gen();		
							// ��������������� ������������� ��������� ��������� ������
		for(int i = optind; i < argc; ++i) {
				
				FILE *in;

				in_pathname = argv[i];
									// ��������� ����
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

							// ����� ����������
		fprintf(stderr, "There are %d record(s), extracted %d record(s).\n", stat_found, stat_extracted);
	
							// ��������� ����������
		if( stat_extracted == stat_found )
				return EXIT_SUCCESS;
		
		return EXIT_FAILURE;
}
/* EOF */
