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

#include "dvda_block.h"
#include "dvda_decipher.h"

#define REMOVE_PROTECTION_CAPTION _T("Decrypting Files")
#define PROGRESS_MIN_RANGE    0
#define PROGRESS_MAX_RANGE 1000

#define MAX_BLOCKS_TO_DECRYPT 512

void dvda_remove_protection_window(HINSTANCE hInst, HWND hwndParent, HWND* phwndMain, HWND* phwndFileName, HWND* phwndProgress) {
	WINDOWINFO wi;
	int x, y, nWidth, nHeight;
	GetWindowInfo(hwndParent, &wi);
	x       = wi.rcWindow.left + wi.cxWindowBorders + 20;
	y       = wi.rcWindow.top + (wi.rcWindow.bottom - wi.rcWindow.top) / 3;
	nWidth  = wi.rcClient.right - wi.rcClient.left - 40;
	nHeight = wi.cyWindowBorders + 80;
	*phwndMain = CreateWindowEx(0, _T("#32770"), REMOVE_PROTECTION_CAPTION, WS_OVERLAPPED, x, y, nWidth, nHeight, hwndParent, NULL, hInst, NULL);
	if (*phwndMain) {
		GetWindowInfo(*phwndMain, &wi);
		x       = 10;
		y       = 5;
		nWidth  = wi.rcClient.right - wi.rcClient.left - 20;
		nHeight = 20;
		*phwndFileName = CreateWindowEx(0, WC_STATIC, NULL, WS_CHILD | WS_VISIBLE, x, y, nWidth, nHeight, *phwndMain, NULL, hInst, NULL);
		y       = 25;
		*phwndProgress = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE, x, y, nWidth, nHeight, *phwndMain, NULL, hInst, NULL);
		SendMessage(*phwndProgress, PBM_SETRANGE, 0, MAKELPARAM(PROGRESS_MIN_RANGE, PROGRESS_MAX_RANGE));
		UpdateWindow(*phwndMain);
	}
	else {
		*phwndFileName = NULL;
		*phwndProgress = NULL;
	}
}

int dvda_remove_protection_from_file(CPxMContext& cpxm_context, LPCTSTR lpszFileName, HWND hwndMain, HWND hwndFileName, HWND hwndProgress) {
	pfc::string_formatter(message);
	message << "Processing " << string_utf8_from_os(lpszFileName);
	SendMessage(hwndFileName, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)string_os_from_utf8(message));
	SendMessage(hwndProgress, PBM_SETPOS, 0, NULL);
	ShowWindow(hwndMain, SW_SHOW);
	int status = -1;
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		status = -2;
		LONGLONG file_size;
		if (GetFileSizeEx(hFile, (PLARGE_INTEGER)&file_size) && file_size >= DVD_BLOCK_SIZE) {
			status = -3;
			HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
			if (hFileMapping != NULL) {
				status = 0;
				LONGLONG file_pos = 0;
				for (;;) {
					if (file_pos >= file_size)
						break;
					int blocks_to_decrypt = (int)((file_size - file_pos) / DVD_BLOCK_SIZE);
					if (blocks_to_decrypt <= 0)
						break;
					if (blocks_to_decrypt > MAX_BLOCKS_TO_DECRYPT)
						blocks_to_decrypt = MAX_BLOCKS_TO_DECRYPT;
					int bytes_to_decrypt = DVD_BLOCK_SIZE * blocks_to_decrypt;
					void* dvd_blocks = (PBYTE)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, ((PLARGE_INTEGER)&file_pos)->HighPart, ((PLARGE_INTEGER)&file_pos)->LowPart, (SIZE_T)bytes_to_decrypt);
					if (dvd_blocks) {
						dvdcpxm_decrypt(&cpxm_context, dvd_blocks, blocks_to_decrypt, DVDCPXM_RESET_CCI);
						UnmapViewOfFile(dvd_blocks);
					}
					else
						status = -4;
					file_pos += blocks_to_decrypt * DVD_BLOCK_SIZE;
					WORD percents = (WORD)(PROGRESS_MIN_RANGE + ((PROGRESS_MAX_RANGE - PROGRESS_MIN_RANGE) * file_pos) / file_size);
					SendMessage(hwndProgress, PBM_SETPOS, percents, NULL);
				}
			}
			CloseHandle(hFileMapping);
		}
	}
	CloseHandle(hFile);
	ShowWindow(hwndMain, SW_HIDE);
	SendMessage(hwndFileName, WM_SETTEXT, 0, NULL);
	SendMessage(hwndProgress, PBM_SETPOS, 0, NULL);
	return status;
}
