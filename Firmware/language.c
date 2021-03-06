//language.c
#include "language.h"
#include <avr/pgmspace.h>
#include "bootapp.h"


#ifdef W25X20CL
#include "w25x20cl.h"
#endif //W25X20CL

// Currently active language selection.
uint8_t lang_selected = 0;


#if (LANG_MODE == 0) //primary language only

uint8_t lang_select(uint8_t lang) { return 0; }
uint8_t lang_get_count() { return 1; }
uint16_t lang_get_code(uint8_t lang) { return LANG_CODE_EN; }
const char* lang_get_name_by_code(uint16_t code) { return _n("English"); }

#else //(LANG_MODE == 0) //secondary languages in progmem or xflash

//reserved xx kbytes for secondary language table
const char _SEC_LANG[LANG_SIZE_RESERVED] PROGMEM_I2 = "_SEC_LANG";

//lang_table pointer
lang_table_t* lang_table = 0;

const char* lang_get_translation(const char* s)
{
	if (lang_selected == 0) return s + 2; //primary language selected, return orig. str.
	if (lang_table == 0) return s + 2; //sec. lang table not found, return orig. str.
	uint16_t ui = pgm_read_word(((uint16_t*)s)); //read string id
	if (ui == 0xffff) return s + 2; //translation not found, return orig. str.
	ui = pgm_read_word(((uint16_t*)(((char*)lang_table + 16 + ui*2)))); //read relative offset
	if (pgm_read_byte(((uint8_t*)((char*)lang_table + ui))) == 0) //read first character
		return s + 2;//zero length string == not translated, return orig. str.
	return (const char*)((char*)lang_table + ui); //return calculated pointer
}

const char* lang_get_sec_lang_str(const char* s)
{
	uint16_t ui = ((((uint16_t)&_SEC_LANG) + 0x00ff) & 0xff00); //table pointer
	lang_table_t* _lang_table = ui; //table pointer
	ui = pgm_read_word(((uint16_t*)s)); //read string id
	if (ui == 0xffff) return s + 2; //translation not found
	ui = pgm_read_word(((uint16_t*)(((char*)_lang_table + 16 + ui*2)))); //read relative offset
	return (const char*)((char*)_lang_table + ui); //return calculated pointer
}

uint8_t lang_select(uint8_t lang)
{
	if (lang == LANG_ID_PRI) //primary language
	{
		lang_table = 0;
		lang_selected = 0;
		return 1;
	}
#ifdef W25X20CL
	if (lang == LANG_ID_SEC) //current secondary language
	{
		uint16_t ui = ((((uint16_t)&_SEC_LANG) + 0x00ff) & 0xff00); //table pointer
		if (pgm_read_dword(((uint32_t*)(ui + 0))) != LANG_MAGIC) return 0; //magic not valid
		lang_table = ui; // set table pointer
		lang_selected = 1; // set language id
		return 1;
	}
#else //W25X20CL
#endif //W25X20CL
/*
	uint16_t ui = (uint16_t)&_SEC_LANG; //pointer to _SEC_LANG reserved space
	ui += 0x00ff; //add 1 page
	ui &= 0xff00; //align to page
	lang_table = ui; //set table pointer
	ui = pgm_read_word(((uint16_t*)(((char*)lang_table + 16)))); //read relative offset of first string (language name)
	return (const char*)((char*)lang_table + ui); //return calculated pointer
*/
	return 0;
}

uint8_t lang_get_count()
{
#ifdef W25X20CL
	uint8_t count = 2; //count = 1+n (primary + secondary + all in xflash)
	uint32_t addr = 0x00000; //start of xflash
	lang_table_header_t header; //table header structure
	while (1)
	{
		w25x20cl_rd_data(addr, (uint8_t*)&header, sizeof(lang_table_header_t)); //read table header from xflash
		if (header.magic != LANG_MAGIC) break; //break if magic not valid
		addr += header.size; //calc address of next table
		count++; //inc counter
	}
	return count;
#else //W25X20CL
#endif //W25X20CL
}

uint16_t lang_get_code(uint8_t lang)
{
#ifdef W25X20CL
	if (lang == LANG_ID_PRI) return LANG_CODE_EN; //primary lang = EN
	if (lang == LANG_ID_SEC)
	{
		uint16_t ui = ((((uint16_t)&_SEC_LANG) + 0x00ff) & 0xff00); //table pointer
		if (pgm_read_dword(((uint32_t*)(ui + 0))) != LANG_MAGIC) return LANG_CODE_XX; //magic not valid
		return pgm_read_word(((uint32_t*)(ui + 10))); //return lang code from progmem
	}
	uint32_t addr = 0x00000; //start of xflash
	lang_table_header_t header; //table header structure
	lang--;
	while (1)
	{
		w25x20cl_rd_data(addr, (uint8_t*)&header, sizeof(lang_table_header_t)); //read table header from xflash
		if (header.magic != LANG_MAGIC) break; //break if not valid
		if (--lang == 0) return header.code;
		addr += header.size; //calc address of next table
	}
#else //W25X20CL
#endif //W25X20CL

//	if (lang == LANG_ID_SEC)
//	{
//		uint16_t ui = ((((uint16_t)&_SEC_LANG) + 0x00ff) & 0xff00); //table pointer
//		if (pgm_read_dword(((uint32_t*)(ui + 0))) == LANG_MAGIC) //magic num is OK
//			return pgm_read_word(((uint16_t*)(ui + 10))); //read language code
//	}
	return LANG_CODE_XX;
}

const char* lang_get_name_by_code(uint16_t code)
{
	switch (code)
	{
	case LANG_CODE_EN: return _n("English");
	case LANG_CODE_CZ: return _n("Cestina");
	case LANG_CODE_DE: return _n("Deutsch");
	case LANG_CODE_ES: return _n("Espanol");
	case LANG_CODE_IT: return _n("Italiano");
	case LANG_CODE_PL: return _n("Polski");
	}
	return _n("??");

//	if (lang == LANG_ID_UNDEFINED) lang = lang_selected;
//	if (lang == LANG_ID_PRI) return _T(MSG_LANGUAGE_NAME + 2);
//	if (lang == LANG_ID_SEC)
//	{
//		uint16_t ui = ((((uint16_t)&_SEC_LANG) + 0x00ff) & 0xff00); //table pointer
//		if (pgm_read_dword(((uint32_t*)(ui + 0))) == LANG_MAGIC) //magic num is OK
//			return lang_get_sec_lang_str(MSG_LANGUAGE_NAME);
//	}
//	return 0;
}

#ifdef DEBUG_SEC_LANG
const char* lang_get_sec_lang_str_by_id(uint16_t id)
{
	uint16_t ui = ((((uint16_t)&_SEC_LANG) + 0x00ff) & 0xff00); //table pointer
	return ui + pgm_read_word(((uint16_t*)(ui + 16 + id * 2))); //read relative offset and return calculated pointer
}

uint16_t lang_print_sec_lang(FILE* out)
{
	printf_P(_n("&_SEC_LANG        = 0x%04x\n"), &_SEC_LANG);
	printf_P(_n("sizeof(_SEC_LANG) = 0x%04x\n"), sizeof(_SEC_LANG));
	uint16_t ptr_lang_table0 = ((uint16_t)(&_SEC_LANG) + 0xff) & 0xff00;
	printf_P(_n("&_lang_table0     = 0x%04x\n"), ptr_lang_table0);
	uint32_t _lt_magic = pgm_read_dword(((uint32_t*)(ptr_lang_table0 + 0)));
	uint16_t _lt_size = pgm_read_word(((uint16_t*)(ptr_lang_table0 + 4)));
	uint16_t _lt_count = pgm_read_word(((uint16_t*)(ptr_lang_table0 + 6)));
	uint16_t _lt_chsum = pgm_read_word(((uint16_t*)(ptr_lang_table0 + 8)));
	uint16_t _lt_resv0 = pgm_read_word(((uint16_t*)(ptr_lang_table0 + 10)));
	uint32_t _lt_resv1 = pgm_read_dword(((uint32_t*)(ptr_lang_table0 + 12)));
	printf_P(_n(" _lt_magic        = 0x%08lx %S\n"), _lt_magic, (_lt_magic==LANG_MAGIC)?_n("OK"):_n("NA"));
	printf_P(_n(" _lt_size         = 0x%04x (%d)\n"), _lt_size, _lt_size);
	printf_P(_n(" _lt_count        = 0x%04x (%d)\n"), _lt_count, _lt_count);
	printf_P(_n(" _lt_chsum        = 0x%04x\n"), _lt_chsum);
	printf_P(_n(" _lt_resv0        = 0x%04x\n"), _lt_resv0);
	printf_P(_n(" _lt_resv1        = 0x%08lx\n"), _lt_resv1);
	if (_lt_magic != LANG_MAGIC) return 0;
	puts_P(_n(" strings:\n"));
	uint16_t ui = ((((uint16_t)&_SEC_LANG) + 0x00ff) & 0xff00); //table pointer
	for (ui = 0; ui < _lt_count; ui++)
		fprintf_P(out, _n("  %3d %S\n"), ui, lang_get_sec_lang_str_by_id(ui));
	return _lt_count;
}
#endif //DEBUG_SEC_LANG

#endif //(LANG_MODE == 0)

//const char MSG_LANGUAGE_NAME[] PROGMEM_I1 = ISTR("English"); ////c=0 r=0

