/*
* DVD-Audio Decoder plugin
* Copyright (c) 2009-2020 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
*
* DVD-Audio Decoder is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* DVD-Audio Decoder is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with FFmpeg; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef _DVDA_DECIPHER_H_INCLUDED
#define _DVDA_DECIPHER_H_INCLUDED

#include <stdint.h>
#include "dvdcpxm.h"
#include "dvda_config.h"

void dvda_remove_protection_window(HINSTANCE hInst, HWND hwndParent, HWND* phwndMain, HWND* phwndFileName, HWND* phwndProgress);
int dvda_remove_protection_from_file(CPxMContext& cpxm_context, LPCTSTR lpszFileName, HWND hwndMain, HWND hwndFileName, HWND hwndProgress);

#endif
