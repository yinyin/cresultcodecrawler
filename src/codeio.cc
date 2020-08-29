#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "chars.h"
#include "codeio.hh"

#define INPUT_BUFFER_SIZE 512

namespace cresultcodecrawler
{
	ResultCodes::ResultCodes(int32_t base_code_value, std::string &code_prefix)
	{
		this->base_code_value = base_code_value;
		this->next_code_value = base_code_value;
		this->code_prefix = code_prefix;
	}

	ResultCodes::~ResultCodes()
	{
	}

	bool ResultCodes::isPrefixWithCodePrefix(std::string &s)
	{
		char *p, *t;
		char ch;
		p = (char *)(this->code_prefix.c_str());
		t = (char *)(s.c_str());
		while ('\0' != (ch = *p))
		{
			char aux = *t;
			p++;
			t++;
			if (ch != aux)
			{
				return false;
			}
		}
		return true;
	}

	int32_t ResultCodes::nextCodeValue()
	{
		int32_t result = this->next_code_value;
		if (this->base_code_value < 0)
		{
			this->next_code_value--;
		}
		else
		{
			this->next_code_value++;
		}
		return result;
	}

	void ResultCodes::addCodeValue(std::string &code_name, int32_t code_value)
	{
		if (0 != this->collected_codes.count(code_name))
		{
			return;
		}
		if (code_value == 0)
		{
			code_value = this->nextCodeValue();
		}
		else
		{
			if (this->base_code_value < 0)
			{
				if (code_value <= this->next_code_value)
				{
					this->next_code_value = code_value - 1;
				}
			}
			else
			{
				if (code_value >= this->next_code_value)
				{
					this->next_code_value = code_value + 1;
				}
			}
		}
		this->collected_codes[code_name] = code_value;
	}

	void ResultCodes::parseDefinitions(char *p, const char *bound)
	{
#define ST_INIT 0
#define ST_PREPROCESS_INDICATE 1
#define ST_IFNDEF 2
#define ST_DEFINE_NAME 3
#define ST_DEFINE_VALUE 4
		char input_buf[INPUT_BUFFER_SIZE];
		int input_idx = 0;
		char ch = '\0';
		int state = ST_INIT;
		std::string code_name;
		/*
		#[1]ifnef [2]...
		#[1]define [3]... [4]...
		*/
		while (p < bound)
		{
			ch = *p;
			p++;
			switch (state)
			{
			case ST_INIT:
				if (ch == '#')
				{
					state = ST_PREPROCESS_INDICATE;
					input_idx = 0;
				}
				break;
			case ST_PREPROCESS_INDICATE:
				if ((ch >= 'a') && (ch <= 'z'))
				{
					if (input_idx < (INPUT_BUFFER_SIZE - 1))
					{
						input_buf[input_idx] = ch;
						input_idx++;
					}
					else
					{
						fprintf(stderr, "WARN: exceed input buffer size @(%s:%d)", __FILE__, __LINE__);
					}
				}
				else if (is_space_character(ch) && (input_idx != 0))
				{
					input_buf[input_idx] = '\0';
					if (0 == strncmp(input_buf, "ifndef", INPUT_BUFFER_SIZE))
					{
						state = ST_IFNDEF;
						input_idx = 0;
					}
					else if (0 == strncmp(input_buf, "define", INPUT_BUFFER_SIZE))
					{
						state = ST_DEFINE_NAME;
						input_idx = 0;
					}
					else
					{
						state = ST_INIT;
						input_idx = 0;
					}
				}
				else
				{
					state = ST_INIT;
					input_idx = 0;
				}
				break;
			case ST_IFNDEF:
				if (is_identifier_character(ch))
				{
					if (input_idx < (INPUT_BUFFER_SIZE - 1))
					{
						input_buf[input_idx] = ch;
						input_idx++;
					}
					else
					{
						fprintf(stderr, "WARN: exceed input buffer size @(%s:%d)", __FILE__, __LINE__);
					}
				}
				else if (((ch == '\n') || is_space_character(ch)) && (input_idx != 0))
				{
					if (this->header_guard.empty())
					{
						input_buf[input_idx] = '\0';
						this->header_guard = input_buf;
					}
					input_idx = 0;
					state = ST_INIT;
				}
				break;
			case ST_DEFINE_NAME:
				if (is_identifier_character(ch))
				{
					if (input_idx < (INPUT_BUFFER_SIZE - 1))
					{
						input_buf[input_idx] = ch;
						input_idx++;
					}
					else
					{
						fprintf(stderr, "WARN: exceed input buffer size @(%s:%d)", __FILE__, __LINE__);
					}
				}
				else if (is_space_character(ch) && (input_idx != 0))
				{
					input_buf[input_idx] = '\0';
					code_name = input_buf;
					input_idx = 0;
					state = ST_DEFINE_VALUE;
				}
				break;
			case ST_DEFINE_VALUE:
				if (is_hex_character(ch))
				{
					if (input_idx < (INPUT_BUFFER_SIZE - 1))
					{
						input_buf[input_idx] = ch;
						input_idx++;
					}
					else
					{
						fprintf(stderr, "WARN: exceed input buffer size @(%s:%d)", __FILE__, __LINE__);
					}
				}
				else if (((ch == '\n') || is_space_character(ch)) && (input_idx != 0))
				{
					if (this->isPrefixWithCodePrefix(code_name))
					{
						int32_t code_value = 0;
						input_buf[input_idx] = '\0';
						code_value = strtol(input_buf, NULL, 0);
						this->addCodeValue(code_name, code_value);
					}
					input_idx = 0;
					code_name.clear();
					state = ST_INIT;
				}
			}
		}
		return;
#undef ST_INIT
#undef ST_PREPROCESS_INDICATE
#undef ST_IFNDEF
#undef ST_DEFINE_NAME
#undef ST_DEFINE_VALUE
	}

	int ResultCodes::LoadDefinitions(std::string &file_path)
	{
		int d_fd;
		off_t d_filesize;
		char *d_raw;
		if (-1 == (d_fd = open(file_path.c_str(), O_RDONLY)))
		{
			if (ENOENT == errno)
			{
				fprintf(stderr, "WARN: definition file not exist [%s] @(%s:%d).\n", file_path.c_str(), __FILE__, __LINE__);
				return 0;
			}
			fprintf(stderr, "ERROR: cannot open definition file for read [%s] @(%s:%d).\n", file_path.c_str(), __FILE__, __LINE__);
			perror("open definition file failed.");
			return 1;
		}
		{
			struct stat statbuf;
			if (-1 == fstat(d_fd, &statbuf))
			{
				perror("ERR: cannot fstat code definition file");
				return 1;
			}
			d_filesize = statbuf.st_size;
			if (MAP_FAILED == (d_raw = (char *)(mmap(NULL, d_filesize, PROT_READ, MAP_SHARED, d_fd, 0))))
			{
				perror("ERR: cannot create mmap");
				return 1;
			}
		}
		this->parseDefinitions(d_raw, d_raw + d_filesize);
		munmap(d_raw, d_filesize);
		close(d_fd);
		return 0;
	}

	void ResultCodes::parseResultCodeNames(char *p, const char *bound)
	{
#define ST_INIT 0
#define ST_IDENTIFIER_S 1
#define ST_COMMENT_S1 2
#define ST_COMMENT_S2BLOCK 3
#define ST_COMMENT_S2LINE 4
#define ST_COMMENT_E1BLOCK 5
#define ST_STRING_N 6
#define ST_STRING_ESCAPE 7
		char input_buf[INPUT_BUFFER_SIZE];
		int input_idx = 0;
		char ch = '\0';
		int state = ST_INIT;
		int line_count = 1;
		/*
		#[1]ifnef [2]...
		#[1]define [3]... [4]...
		*/
		while (p < bound)
		{
			ch = *p;
			p++;
			switch (state)
			{
			case ST_INIT:
				if (((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')))
				{
					if (input_idx < (INPUT_BUFFER_SIZE - 1))
					{
						input_buf[input_idx] = ch;
						input_idx++;
					}
					else
					{
						fprintf(stderr, "WARN: exceed input buffer size @(%s:%d)", __FILE__, __LINE__);
					}
					state = ST_IDENTIFIER_S;
				}
				else if (ch == '/')
				{
					state = ST_COMMENT_S1;
				}
				else if (ch == '"')
				{
					state = ST_STRING_N;
				}
				break;
			case ST_IDENTIFIER_S:
				if (is_identifier_character(ch))
				{
					if (input_idx < (INPUT_BUFFER_SIZE - 1))
					{
						input_buf[input_idx] = ch;
						input_idx++;
					}
					else
					{
						fprintf(stderr, "WARN: exceed input buffer size @(%s:%d)", __FILE__, __LINE__);
					}
				}
				else if (is_delim_character(ch))
				{
					if (input_idx > 0)
					{
						input_buf[input_idx] = '\0';
						std::string n = input_buf;
						if (this->isPrefixWithCodePrefix(n))
						{
							this->addCodeValue(n, 0);
							printf("INFO: found result code symbol [%s] (line:%d)\n", input_buf, line_count);
						}
						input_idx = 0;
					}
					state = ST_INIT;
				}
				else if (ch == '"')
				{
					input_idx = 0;
					state = ST_STRING_N;
				}
				else
				{
					input_idx = 0;
					state = ST_INIT;
				}
				break;
			case ST_COMMENT_S1:
				if (ch == '*')
				{
					state = ST_COMMENT_S2BLOCK;
				}
				else if (ch == '/')
				{
					state = ST_COMMENT_S2LINE;
				}
				else
				{
					state = ST_INIT;
				}
				break;
			case ST_COMMENT_S2BLOCK:
				if (ch == '*')
				{
					state = ST_COMMENT_E1BLOCK;
				}
				break;
			case ST_COMMENT_S2LINE:
				if (ch == '\n')
				{
					state = ST_INIT;
				}
				break;
			case ST_COMMENT_E1BLOCK:
				if (ch == '/')
				{
					state = ST_INIT;
				}
				else if (ch == '*')
				{
					state = ST_COMMENT_E1BLOCK;
				}
				break;
			case ST_STRING_N:
				if (ch == '\\')
				{
					state = ST_STRING_ESCAPE;
				}
				else if (ch == '"')
				{
					state = ST_INIT;
				}
				break;
			case ST_STRING_ESCAPE:
				state = ST_STRING_N;
				break;
			}
			if ('\n' == ch)
			{
				line_count++;
			}
		}
		return;
#undef ST_INIT
#undef ST_IDENTIFIER_S
#undef ST_COMMENT_S1
#undef ST_COMMENT_S2BLOCK
#undef ST_COMMENT_S2LINE
#undef ST_COMMENT_E1BLOCK
#undef ST_STRING_N
#undef ST_STRING_ESCAPE
	}

	int ResultCodes::loadResultCodeNames(std::string &file_path)
	{
		int d_fd;
		off_t d_filesize;
		char *d_raw;
		if (-1 == (d_fd = open(file_path.c_str(), O_RDONLY)))
		{
			fprintf(stderr, "ERROR: cannot open source file for read [%s] @(%s:%d).\n", file_path.c_str(), __FILE__, __LINE__);
			perror("open source file failed.");
			return 1;
		}
		{
			struct stat statbuf;
			if (-1 == fstat(d_fd, &statbuf))
			{
				perror("ERR: cannot fstat source file");
				return 1;
			}
			d_filesize = statbuf.st_size;
			if (MAP_FAILED == (d_raw = (char *)(mmap(NULL, d_filesize, PROT_READ, MAP_SHARED, d_fd, 0))))
			{
				perror("ERR: cannot create mmap");
				return 1;
			}
		}
		this->parseResultCodeNames(d_raw, d_raw + d_filesize);
		munmap(d_raw, d_filesize);
		close(d_fd);
		return 0;
	}

	int ResultCodes::LoadResultCodeNames(std::vector<std::string> &file_paths)
	{
		for (auto file_path : file_paths)
		{
			int ret_code;
			printf("INFO: loading result code names from [%s]\n", file_path.c_str());
			if (0 != (ret_code = this->loadResultCodeNames(file_path)))
			{
				fprintf(stderr, "ERROR: having error on load result code names from [%s]: %d\n", file_path.c_str(), ret_code);
			}
		}
		return 0;
	}

	void ResultCodes::PrintCollectedCodes()
	{
		fprintf(stderr, "DEBUG: header guard: %s.\n", this->header_guard.c_str());
		for (auto c : this->collected_codes)
		{
			fprintf(stderr, "DEBUG: %s => %d.\n", c.first.c_str(), int(c.second));
		}
		return;
	}
} // namespace cresultcodecrawler
