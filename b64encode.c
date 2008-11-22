#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "base64.h"

#ifdef _WIN32
typedef int mode_t;
#endif //_WIN32

const char *nl = "\n";
const size_t nl_len = sizeof("\n") - 1;

const char *argv_0;

void encode(const char *name, const mode_t mode, FILE *in, FILE *out) {
	char in_buf[ 45 ];
	char out_buf[ base64_length(45) ];
	size_t rec;

	if( 0 > fprintf(out, "%s %o %s\n", "begin-base64", mode, name) )
		goto _fail_io;
	while( sizeof(in_buf) == (rec = fread(in_buf, 1, sizeof(in_buf), in)) ) {

		base64_encode(in_buf, sizeof(in_buf), out_buf, sizeof(out_buf));

		if( 1 != fwrite(out_buf, sizeof(out_buf), 1, out) )
			goto _fail_io;

		if( 1 != fwrite(nl, nl_len, 1, out) )
			goto _fail_io;
	}

	if( rec > 0 ) {
		rec = base64_encode(in_buf, rec, out_buf, sizeof(out_buf));

		if( 1 != fwrite(out_buf, rec, 1, out) )
			goto _fail_io;

		if( 1 != fwrite(nl, nl_len, 1, out) )
			goto _fail_io;
	}

	if( ferror(in) )
		goto _fail_io;

	if( 1 != fwrite("====", 4, 1, out) )
		goto _fail_io;

	if( 1 != fwrite(nl, nl_len, 1, out) )
		goto _fail_io;

	return;

 _fail_io:
	fprintf(stderr, "%s: %s\n", argv_0, strerror(errno));
	exit(-1);

}

void usage() {
	fprintf(stderr, "%s%s%s", "Usage: ", argv_0, " INFILE [ OUTFILE ]\n");
	fprintf(stderr, "%s", "\t\t( INFILE != OUTFILE ) must be there!\n\n");
	fprintf(stderr, "%s", "Bugs and Your Ideas mailto the.zett@gmail.com\n");
	exit(-1);
}

int main(int argc, const char **argv) {
	FILE *in = stdin;
	FILE *out = stdout;
	struct stat status;

	argv_0 = argv[0];

	if( 2 != argc && 3 != argc )
		usage();

	if( NULL == (in = fopen(argv[1], "rb")) ) {
		fprintf(stderr, "%s: open for read \"%s\": %s\n", argv_0, argv[1], strerror(errno));
		return -1;
	}

	if( 3 == argc && NULL == (out = fopen(argv[2], "wb")) ) {
		fprintf(stderr, "%s: open for write \"%s\": %s\n", argv_0, argv[2], strerror(errno));
		return -1;
	}

	if( -1 == fstat(fileno(in), &status) ) {
		fprintf(stderr, "%s: take access mode of \"%s\": %s\n", argv_0, argv[1], strerror(errno));
		return -1;
	}

	encode(argv[1], status.st_mode & 0777, in, out);

	return 0;
}

