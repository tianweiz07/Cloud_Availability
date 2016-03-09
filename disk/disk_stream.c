#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>


int Bsize, count, gotcnt, int_count;
int nbufs = 1;
int in, out;

int getfile(char *s, int argc, char **argv) {
	register int ret, len, i;
	int oflags;

	len = strlen(s);

	for (i = 1; i < argc; ++i) {
		if (!strncmp(argv[i], s, len)) {
			if (argv[i][0] == 'o') {
				if (!strcmp("of=internal", argv[i]))
					return (-2);

				oflags = O_WRONLY | O_TRUNC | O_CREAT;
				ret = open(&argv[i][len], oflags,0644);
				if (ret == -1)
					error(&argv[i][len]);
				return (ret);

			} else {
				if (!strcmp("if=internal", argv[i]))
					return (-2);
				ret = open(&argv[i][len], 0);
				if (ret == -1)
					error(&argv[i][len]);
				return (ret);
			}
		}
	}
	return (-2);
}

int main(int argc, char **argv)
{
	uint  *buf;
	uint  *bufs[10];
	int nextbuf = 0;

	Bsize = atoi(argv[1]);
	if (Bsize < 0)
		Bsize = 8192;
	count = atoi(argv[2]);
	gotcnt = 1;
	int_count = 0;

	int i;
	for (i = 0; i < nbufs; i++) {
		if (!(bufs[i] = (uint *) valloc((unsigned) Bsize))) {
			perror("VALLOC");
			exit(1);
		}
		bzero((char *) bufs[i], Bsize);
	}

	in = getfile("if=", argc, argv);
	out = getfile("of=", argc, argv);

	for (;;) {
		register int moved;

		if (gotcnt && count-- <= 0) {
			fsync(out);
			exit(0);
		}

		buf = bufs[nextbuf];
		if (++nextbuf == nbufs) nextbuf = 0;
		if (in >= 0) {
			moved = read(in, buf, Bsize);
		} else {
			moved = Bsize;
		}
		if (moved == -1) {
			perror("read");
		}
		if (moved <= 0) {
			fsync(out);
			exit(0);
		}

		if (out >= 0) {
			int moved2;

			moved2 = write(out, buf, moved);

			if (moved2 == -1) {
				perror("write");
			}
			if (moved2 != moved) {
				fprintf(stderr, "write: wanted=%d got=%d\n", moved, moved2);
				fsync(out);
				exit(0);
			}

		}

		int_count += (moved >> 2);
	}
}
