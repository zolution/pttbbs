/* $Id$ */
/* tool to convert wretch bbs to ptt bbs */

#include "bbs.h"

typedef struct {
    time4_t chrono;
    char pad[4];
    int xmode;
    int xid;
    char xname[32];               /* �ɮצW�� */
    char owner[80];               /* �@�� (E-mail address) */
    char nick[50];                /* �ʺ� */
    char date[9];                 /* [96/12/01] */
    char title[72];               /* �D�D (TTLEN + 1) */
    char score;
    char pad2[4];
} wretch_HDR;

#define WR_POST_MARKED     0x00000002      /* marked */
#define WR_POST_BOTTOM1    0x00002000      /* �m���峹������ */

#define WR_GEM_RESTRICT    0x0800          /* ����ź�ذϡA�� manager �~��� */
#define WR_GEM_RESERVED    0x1000          /* ����ź�ذϡA�� sysop �~���� */
#define WR_GEM_FOLDER      0x00010000      /* folder / article */
#define WR_GEM_BOARD       0x00020000      /* �ݪO��ذ� */

static const char radix32[32] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
};

char *path;
int trans_man = 0;

int convert(char *fname, char *newpath)
{
    int fd;
    char *p;
    char buf[PATHLEN];
    wretch_HDR whdr;
    fileheader_t fhdr;

    if (fname[0] == '.')
	snprintf(buf, sizeof(buf), "%s/%s", path, fname);
    else
	snprintf(buf, sizeof(buf), "%s/%c/%s", path, fname[7], fname);

    if ((fd = open(buf, O_RDONLY, 0)) == -1)
	return -1;

    while (read(fd, &whdr, sizeof(whdr)) == sizeof(whdr)) {
	if (strcmp(whdr.xname, "..") == 0 || strchr(whdr.xname, '/'))
	    continue;

	if (!(whdr.xmode & 0xffff0000)) {
	    /* article */
	    stampfile(newpath, &fhdr);

	    if (trans_man)
		strlcpy(fhdr.title, "�� ", sizeof(fhdr.title));

	    if (whdr.xname[0] == '@')
		snprintf(buf, sizeof(buf), "%s/@/%s", path, whdr.xname);
	    else
		snprintf(buf, sizeof(buf), "%s/%c/%s", path, whdr.xname[7], whdr.xname);

	    fhdr.modified = dasht(buf);
	    if (whdr.xmode & WR_POST_MARKED)
		fhdr.filemode |= FILE_MARKED;

	    copy_file(buf, newpath);
	}
	else if (whdr.xmode & WR_GEM_FOLDER) {
	    /* folder */
	    stampdir(newpath, &fhdr);

	    if (trans_man)
		strlcpy(fhdr.title, "�� ", sizeof(fhdr.title));

	    convert(whdr.xname, newpath);
	}
	else {
	    /* ignore */
	    continue;
	}

	p = strrchr(newpath, '/');
	*p = '\0';

	if (whdr.xmode & WR_GEM_RESTRICT)
	    fhdr.filemode |= FILE_BM;

	strlcat(fhdr.title, whdr.title, sizeof(fhdr.title));
	strlcpy(fhdr.owner, whdr.owner, sizeof(fhdr.owner));
	if (whdr.date[0] && strlen(whdr.date) == 8)
	    strlcpy(fhdr.date, whdr.date + 3, sizeof(fhdr.date));
	else
	    strlcpy(fhdr.date, "     ", sizeof(fhdr.date));

	setadir(buf, newpath);
	append_record(buf, &fhdr, sizeof(fhdr));
    }

    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    char buf[PATHLEN];
    char *board;
    int opt;

    while ((opt = getopt(argc, argv, "m")) != -1) {
	switch (opt) {
	    case 'm':
		trans_man = 1;
		break;
	    default:
		fprintf(stderr, "%s [-m] <path> <board>\n", argv[0]);
		fprintf(stderr, "  -m convert man (default is board)\n");
	}
    }

    if ((argc - optind + 1) < 2) {
	fprintf(stderr, "%s [-m] <path> <board>\n", argv[0]);
	fprintf(stderr, "  -m convert man (default is board)\n");
	return 0;
    }


    path = argv[1];
    board = argv[2];

    if (!dashd(path)) {
	fprintf(stderr, "%s is not directory\n", path);
	return 0;
    }

    attach_SHM();
    opt = getbnum(board);

    if (opt == 0) {
	fprintf(stderr, "ERR: board `%s' not found\n", board);
	return 0;
    }

    if (trans_man)
	setapath(buf, board);
    else
	setbpath(buf, board);

    if (!dashd(buf)) {
	fprintf(stderr, "%s is not directory\n", buf);
	return 0;
    }

    attach_SHM();

    convert(".DIR", buf);

    if (trans_man) {
	strlcat(buf, "/.rebuild", sizeof(buf));
	if ((opt = open(buf, O_CREAT, 0640)) > 0)
	    close(opt);
    }
    else
	touchbtotal(opt);

    return 0;
}