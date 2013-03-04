/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2010 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

int read_one_line_file(const char *fn, char **line) {
        FILE *f;
        int r;
        char t[LINE_MAX], *c;

        assert(fn);
        assert(line);

        f = fopen(fn, "re");
        if (!f)
                return -errno;

        if (!fgets(t, sizeof(t), f)) {

                if (ferror(f)) {
                        r = -errno;
                        goto finish;
                }

                t[0] = 0;
        }

        c = strdup(t);
        if (!c) {
                r = -ENOMEM;
                goto finish;
        }

        truncate_nl(c);

        *line = c;
        r = 0;

finish:
        fclose(f);
        return r;
}

int running_in_chroot(void) {
        struct stat a, b;

        zero(a);
        zero(b);

        /* Only works as root */

        if (stat("/proc/1/root", &a) < 0)
                return -errno;

        if (stat("/", &b) < 0)
                return -errno;

        return
                a.st_dev != b.st_dev ||
                a.st_ino != b.st_ino;
}

bool startswith(const char *s, const char *prefix) {
        size_t sl, pl;

        assert(s);
        assert(prefix);

        sl = strlen(s);
        pl = strlen(prefix);

        if (pl == 0)
                return true;

        if (sl < pl)
                return false;

        return memcmp(s, prefix, pl) == 0;
}

char *truncate_nl(char *s) {
        assert(s);

        s[strcspn(s, NEWLINE)] = 0;
        return s;
}


