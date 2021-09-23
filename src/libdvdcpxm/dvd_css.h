#pragma once

#include <stdint.h>
#include <windows.h>

#include "dvd_command.h"

#define KEY_SIZE 5

typedef uint8_t dvd_key_t[KEY_SIZE];

typedef struct
{
	int       protection;
	int       agid;
	dvd_key_t p_bus_key;
	dvd_key_t p_disc_key;
	dvd_key_t p_title_key;
} css_t;

extern css_t css;

extern int  GetDiscKey    (void);
extern int  GetBusKey     (void);
extern int  GetASF        (void);
extern void CryptKey      (int i_key_type, int i_variant, uint8_t *p_challenge, uint8_t *p_key);
extern void DecryptKey    (uint8_t invert, uint8_t *p_key, uint8_t *p_crypted, uint8_t *p_result);
extern int  DecryptDiscKey(uint8_t *p_struct_disckey, dvd_key_t p_disc_key);
