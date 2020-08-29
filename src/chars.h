#ifndef _C_RESULT_CODE_CRAWLER_CHARS_H_
#define _C_RESULT_CODE_CRAWLER_CHARS_H_ 1

#ifdef __cplusplus
extern "C"
{
#endif

	int is_identifier_character(char ch);

	int is_space_character(char ch);

	int is_hex_character(char ch);

	int is_delim_character(char ch);

#ifdef __cplusplus
}
#endif

#endif /* _C_RESULT_CODE_CRAWLER_CHARS_H_ */
