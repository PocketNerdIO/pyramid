/* global.h - some definitions required by both the .rs and the .c files
 */

/*
    Pyramid, a Patience Game for the Psion Series 3
    Version 1.0a
    Copyright (C) 1993  J Cade Roux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 * See the file COPYING for the GNU General Public License.
 * Mail: Cade Roux
 *			c/o Dubroca
 *			Box 513
 *			Boutte, LA  70039
 *			USA
 */

#ifndef BOOL
	#define BOOL	INT
#endif

#ifndef TRUE
	#define TRUE	1
#endif

#ifndef FALSE
	#define FALSE	0
#endif

#ifndef MIN
	#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
	#define MAX(a,b)	((a) > (b) ? (a) : (b))
#endif

#define VERSION_STRING	"1.0a"
