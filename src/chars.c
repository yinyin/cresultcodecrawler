#include "chars.h"

int is_identifier_character(char ch)
{
	if (
		((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || ((ch >= '0') && (ch <= '9')) || (ch == '_') || (ch >= 0x80))
	{
		return 1;
	}
	return 0;
}

int is_space_character(char ch)
{
	switch (ch)
	{
	case ' ':
	case '\t':
	case '\r':
		return 1;
	}
	return 0;
}

int is_hex_character(char ch)
{
	if (((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'f')) || ((ch >= 'A') && (ch <= 'F')) || (ch == 'x') || (ch == '-'))
	{
		return 1;
	}
	return 0;
}

int is_delim_character(char ch)
{
	const static char *delim_chars = "; \t=+-*/%!><&|~^[]{}()?:\r\n";
	char t, *p;
	p = (char *)delim_chars;
	while ('\0' != (t = *p))
	{
		p++;
		if (t == ch)
		{
			return 1;
		}
	}
	return 0;
}
