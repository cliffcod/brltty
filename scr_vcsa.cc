/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/* scr_vcsa.cc - screen library for use with Linux vcsa devices.
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
 */

#define SCR_C 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <errno.h>

#include "misc.h"

#include "scr.h"
#include "scrdev.h"
#include "scr_vcsa.h"
#include "config.h"



/* Instanciation of the vcsa driver here */
static vcsa_Screen vcsa;

/* Pointer for external reference */
RealScreen *live = &vcsa;



// Conversion from ibmpc==cp437 to iso-latin-1 for chars>=128
static const unsigned char cp437_to_latin1[128] =
{199, 252, 233, 226, 228, 224, 229, 231, 234, 235, 232, 239, 238, 236,
 196, 197, 201, 230, 198, 244, 246, 242, 251, 249, 255, 214, 220, 162,
 163, 165, 158, 159, 225, 237, 243, 250, 241, 209, 170, 186, 191, 169,
 172, 189, 188, 161, 171, 187, 248, 164, 253, 179, 180, 145, 20, 156,
 184, 185, 21, 175, 166, 174, 190, 168, 192, 193, 194, 195, 142, 143,
 146, 128, 200, 144, 202, 203, 204, 205, 206, 207, 208, 157, 210, 211,
 212, 213, 153, 215, 216, 217, 218, 219, 154, 221, 222, 152, 133, 223,
 131, 227, 132, 134, 181, 135, 138, 130, 136, 137, 141, 173, 140, 139,
 240, 177, 149, 155, 147, 245, 247, 148, 176, 151, 183, 150, 129, 178,
 254, 160};


/* 
 * The virtual console devices returns the actual font value used on the 
 * screen.  Most PC video cards have built-in fonts defined for the CP437
 * character set, but Linux (and most UNIXes) operates in  the ISO-Latin-1 
 * character set.  So the kernel translates characters to be printed on the
 * screen into CP437 character set to accomodate PC video cards.  When this,
 * happens, we need to translate the screen capture back into ISO characters.
 * 
 * Since CP437 isn't used on all display devices, we need to probe the display
 * to determine which character set is in use.  To accomplish this, we just
 * print a well known character on the screen, and deduce the character set
 * in use by looking at the resulting font index value.
 */

void vcsa_Screen::set_screen_translation_table (void) 
{
  unsigned short old_character;	/* 'short' to hold char and attr */
  unsigned char new_character = 0;
  int tty_fd;

  if ((tty_fd = ::open (CONSOLE, O_WRONLY)) == -1) {
    LogAndStderr(LOG_WARNING,"Can't open console device '%s'for writing: %s\n",
		 CONSOLE, strerror(errno));
  } else {
    // Save the current cursor position.
    {
	static const unsigned char save_cursor[] = {0X1B, '7'};
	write(tty_fd, save_cursor, sizeof(save_cursor));
    }

    // Move the cursor to the top-left character.
    {
	static const unsigned char home[] = {0X1B, '[', 'H'};
	write(tty_fd, home, sizeof(home));
    }

    // Get the current character at this position.
    lseek(fd, 4, SEEK_SET);
    read(fd, &old_character, 2);

    // Change it to an ISO "nobreakspace".
    new_character = 0xA0;
    write(tty_fd, &new_character, 1);

    // Get the new value of the character.
    lseek(fd, -2, SEEK_CUR);
    read(fd, &new_character, 1);

    // Restore the character to its original value.
    lseek(fd, -1, SEEK_CUR);
    write(fd, &old_character, 2);

    // Restore the cursor to its original position.
    {
	static const unsigned char restore_cursor[] = {0X1B, '8'};
	write(tty_fd, restore_cursor, sizeof(restore_cursor));
    }

    ::close(tty_fd);
  }

  // Determine which translation table to use when reading the screen.
  {
	int level = LOG_INFO;
	const char *font;
	switch (new_character) {
	  default: // no translation
	    level = LOG_WARNING;
	    translation_table = NULL;
	    font = "unknown";
	    break;
	  case 0XA0: // ISO-LATIN-1
	    translation_table = NULL;
	    font = "ISO-LATIN-1";
	    break;
	  case 0XFF: // CP-437
	    translation_table = cp437_to_latin1;
	    font = "CP-437";
	    break;
	}
	LogAndStderr(level, "Screen Font: %s", font);
  }
}


int vcsa_Screen::open (void)
{
  fd = -1;
  if ((fd = ::open (VCSADEV, O_RDONLY)) == -1){
#if 0
    if(errno == ENOENT){
      LogAndStderr(LOG_WARNING, "Can't find vcsa device '%s'. Creating it.",
		   VCSADEV);
      if(mknod(VCSADEV, S_IFCHR | 0600, (7<<8)|128) == 0)
	fd = ::open (VCSADEV, O_RDONLY);
      else
	LogAndStderr(LOG_WARNING, "mknod: %s", strerror(errno));
    }
#endif
  }
  if(fd<0){
    LogAndStderr(LOG_WARNING,"Can't open vcsa device '%s': %s\n",
		 VCSADEV, strerror(errno));
    return 1;
  }
  if ((cons_fd = ::open (CONSOLE, O_RDONLY)) == -1)
    {
      LogAndStderr(LOG_WARNING,"Can't open console device '%s': %s\n",
		 CONSOLE, strerror(errno));
      ::close (fd);
      return 1;
    }
  set_screen_translation_table();
  return 0;
}


void vcsa_Screen::getstat (scrstat & stat)
{
  unsigned char buffer[4];
  struct vt_stat vtstat;

  lseek (fd, 0, SEEK_SET);	/* go to start of file */
  read (fd, buffer, 4);		/* get screen status bytes */
  stat.rows = buffer[0];
  stat.cols = buffer[1];
  stat.posx = buffer[2];
  stat.posy = buffer[3];
  ioctl (cons_fd, VT_GETSTATE, &vtstat);
  stat.no = vtstat.v_active;
}


unsigned char * 
vcsa_Screen::getscr (winpos pos, unsigned char *buffer, short mode)
{
  /* screen statistics */
  scrstat stat;
  getstat (stat);
  if (pos.left < 0 || pos.top < 0 || pos.width < 1 || pos.height < 1 \
      ||mode < 0 || mode > 1 || pos.left + pos.width > stat.cols \
      ||pos.top + pos.height > stat.rows)
    return NULL;
  /* start offset */
  off_t start = 4 + (pos.top * stat.cols + pos.left) * 2 + mode;
  /* number of bytes to read for one complete line */
  size_t linelen = 2 * pos.width - 1;
  /* line buffer */
  unsigned char linebuf[linelen];
  /* pointer to copy data to */
  unsigned char *dst = buffer;
  /* pointer to copy data from */
  unsigned char *src;
  /* indexes */
  int i, j;

  for (i = 0; i < pos.height; i++)
    {
      lseek (fd, start + i * stat.cols * 2, SEEK_SET);
      read (fd, linebuf, linelen);
      src = linebuf;
      for (j = 0; j < pos.width; j++)
	{
	  if ((mode == SCR_TEXT) && (*src >= 128) && (translation_table != NULL))
	    /* Conversion to iso-latin-1 for chars>=128 */
	    *dst++ = translation_table[(*src) - 128];
	  else
	    *dst++ = *src;
	  src += 2;
	}
    }
  return buffer;
}


void vcsa_Screen::close (void)
{
  ::close (fd);
  ::close (cons_fd);
}
