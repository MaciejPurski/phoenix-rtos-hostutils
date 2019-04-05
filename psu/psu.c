/*
 * Phoenix-RTOS
 *
 * Phoenix server
 *
 * Copyright 2001, 2004 Pawel Pisarczyk
 * Copyright 2012 Phoenix Systems
 *
 * Author: Pawel Pisarczyk, Jacek Popko
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <stdlib.h>
#include <getopt.h>

#include "../common/types.h"
#include "../common/errors.h"
#include "../common/serial.h"
#include "../phoenixd/bsp.h"
#include "../phoenixd/msg_udp.h"
#include "../phoenixd/dispatch.h"

#include <hidapi/hidapi.h>

enum {
	SDP
};

extern char *optarg;


#define VERSION "1.3"


void usage(char *progname)
{
	printf("Usage: %s script_path\n", progname);
}


int phoenixd_session(char *tty, char *kernel, char *sysdir)
{
	u8 t;
	int fd, count, err;
	u8 buff[BSP_MSGSZ];

	fprintf(stderr, "[%d] Starting phoenixd-child on %s\n", getpid(), tty);

	if ((fd = serial_open(tty, B460800)) < 0) {
		fprintf(stderr, "[%d] Can't open %s [%d]!\n", getpid(), tty, fd);
		return ERR_PHOENIXD_TTY;
	}

	for (;;) {
		if ((count = bsp_recv(fd, &t, (char*)buff, BSP_MSGSZ, 0)) < 0) {
			bsp_send(fd, BSP_TYPE_RETR, NULL, 0);
			continue;
		}

		switch (t) {

		/* Handle kernel request */
		case BSP_TYPE_KDATA:
			if (*(u8 *)buff != 0) {
				fprintf(stderr, "[%d] Bad kernel request on %s\n", getpid(), tty);
				break;
			}
			fprintf(stderr, "[%d] Sending kernel to %s\n", getpid(), tty);

			if ((err = bsp_sendkernel(fd, kernel)) < 0) {
				fprintf(stderr, "[%d] Sending kernel error [%d]!\n", getpid(), err);
				break;
			}
			break;

		/* Handle program request */
		case BSP_TYPE_PDATA:
			fprintf(stderr, "[%d] Load program request on %s, program=%s\n", getpid(), tty, &buff[2]);
			if ((err = bsp_sendprogram(fd, (char*)&buff[2], sysdir)) < 0)
				fprintf(stderr, "[%d] Sending program error [%d]!\n", getpid(), err);
			break;
		}
	}
	return 0;
}


hid_device *open_device_with_vid_pid(uint16_t vid, uint16_t pid)
{
    hid_device* h = NULL;
	struct hid_device_info* list = hid_enumerate(vid, pid); // Find all devices with given vendor

	for (struct hid_device_info* it = list; it != NULL; it = it->next) {
		if ((h = hid_open_path(it->path)) == NULL) {
			fprintf(stderr, "Failed to open device\n");
			continue;
		} else {
			break;
		}
	}

	if (list)
		hid_free_enumeration(list);

	return h;
}


int wait_cmd(hid_device **dev)
{
	int retries = 0;
	while ((*dev = open_device_with_vid_pid(0x15a2, 0x0)) == NULL) {
		if (retries++ > 10)
			return -1;
		sleep(1);
	}

	return 0;
}


int decode_line(char *line, size_t len, size_t lineno)
{
	int err = 0;
	char *tok = strtok(line, " ");
	size_t toklen = strlen(tok);

	if (tok[0] != '\n' && tok[0] != '#') { /* Skip empty lines and comments */
		if (tok[toklen - 1] == '\n') {
			tok[--toklen] = '\0';
		}

		fprintf(stderr, "Parsing %lu: '%s'\n", lineno, tok);

		if (!strcmp(tok, "WAIT")) {

		} else if(!strcmp(tok, "WRITE_FILE")) {
		} else if(!strcmp(tok, "REBOOT")) {
		} else if(!strcmp(tok, "ARGS")) {
		} else if(!strcmp(tok, "JUMP_ADDRESS")) {
		} else if(!strcmp(tok, "DCD_WRITE")) {
		} else if(!strcmp(tok, "PROMPT")) {
		} else if(!strcmp(tok, "REBOOT")) {
		} else {
			fprintf(stderr, "Unrecognized token '%s' at line %lu\n", tok, lineno);
			err = -1;
		}
	}
	return err;
}


int main(int argc, char *argv[])
{
	FILE *script;
	int res;
	size_t len = 1024, lineno = 0;
	char *buff = malloc(len);
	hid_device *dev;

	if (argc != 2) {
		usage(argv[0]);
		return -1;
	}

	/* Interpret script */
	script = fopen(argv[1], "r");
	while (script != NULL && (res = getline(&buff, &len, script)) > 0) {
		if ((res = decode_line(buff, res, lineno++)) < 0) {
			res = -1;
			break;
		}
	}

	free(buff);
	if (script) {
		fclose(script);
	}
	return res;
}

#if 0
int main_old(int argc, char *argv[])
{
	int c;
	int ind;
	int len, len2;
	char bspfl = 0;
	char *kernel = "../kernel/phoenix";

	int sdp = 0;
	int help = 0;
	int opt_idx = 0;
	char *initrd = NULL;
	char *console = NULL;
	char *append = NULL;
	char *output = NULL;
	void *dev = NULL;

	char *sysdir = "../sys";
	char *ttys[8];
	mode_t mode[8] = {SERIAL};
	int k, i = 0;
	int res, st;
	int type, proto;

	struct option long_opts[] = {
		{"sdp", no_argument, &sdp, 1},
		{"plugin", no_argument, &sdp, 2},
		{"upload", no_argument, &sdp, 3},
		{"kernel", required_argument, 0, 'k'},
		{"console", required_argument, 0, 'c'},
		{"initrd", required_argument, 0, 'I'},
		{"append", required_argument, 0, 'a'},
		{"execute", required_argument, 0, 'x'},
		{"help", no_argument, &help, 1},
		{"output", required_argument, 0, 'o'},
		{0, 0, 0, 0}};

	while (1) {
		c = getopt_long(argc, argv, "k:p:s:1m:i:u:a:x:c:I:", long_opts, &opt_idx);
		if (c < 0)
			break;

		switch (c) {
		case 'd':
			kernel = optarg;
			break;
		case 'h':
			sysdir = optarg;
			break;
		default:
			break;
		}
	}

	switch (type) {
	}

	if (proto == SDP) {
		sdp_execute(dev);
	}

	return 0;
}
#endif