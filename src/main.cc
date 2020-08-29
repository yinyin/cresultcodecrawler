#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>

#include <getopt.h>

#include "chars.h"
#include "codeio.hh"
#include "errors.h"

static int check_result_code_prefix(char *p, char *p_last_ch)
{
	char ch;
	char last_ch = '\0';
	int len = 0;
	while ('\0' != (ch = *p))
	{
		p++;
		if (is_identifier_character(ch))
		{
			last_ch = ch;
			len++;
			continue;
		}
		return CRESTCODE_ARGERR_ILLEGAL_IDENT_CHAR;
	}
	if (len == 0)
	{
		return CRESTCODE_ARGERR_EMPTY_IDENT_CHAR;
	}
	*p_last_ch = last_ch;
	return 0;
}

static int parse_argv(std::string &output_path, std::string &code_prefix, std::vector<std::string> &input_paths, int32_t &base_value, bool &gen_text_func, int argc, char *argv[])
{
	static struct option long_options[] = {
		{"output", required_argument, NULL, 'o'},
		{"prefix", required_argument, NULL, 'p'},
		{"base", required_argument, NULL, 'b'},
		{"text", no_argument, NULL, 'T'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}};
	int c;
	int option_index = 0;
	output_path.clear();
	code_prefix.clear();
	input_paths.clear();
	base_value = 0;
	gen_text_func = false;
	while (-1 != (c = getopt_long(argc, argv, "o:p:b:Th", long_options, &option_index)))
	{
		switch (c)
		{
		case 'o':
		{
			char *p;
			if (NULL == (p = realpath(optarg, NULL)))
			{
				perror("cannot expand given output path");
				return CRESTCODE_ARGERR_EXPAND_DEF_FILE_PATH;
			}
			output_path = p;
			free(p);
		}
		break;
		case 'p':
		{
			char last_ch = '\0';
			if (0 != check_result_code_prefix(optarg, &last_ch))
			{
				fprintf(stderr, "ERROR: option `--prefix=` need contain identifier characters: [%s].\n", optarg);
				return CRESTCODE_ARGERR_PREFIX_NEED_IDENT_CHARS;
			}
			code_prefix = optarg;
			if (last_ch != '_')
			{
				code_prefix += '_';
			}
		}
		break;
		case 'b':
		{
			int32_t v;
			char *remain = NULL;
			v = (int32_t)(strtol(optarg, &remain, 0));
			if (*remain == '\0')
			{
				base_value = v;
			}
			else
			{
				fprintf(stderr, "ERROR: option `--base=` need have valid code value: [%s].\n", optarg);
				return CRESTCODE_ARGERR_INVALID_BASE_CODE_VALUE;
			}
		}
		break;
		case 'T':
		{
			gen_text_func = true;
		}
		break;
		case 'h':
			printf(
				"Argument:\n"
				"    --output=[OUTPUT_PATH] | -o [OUTPUT_PATH]\n"
				"        Path of result code header (.h) file.\n"
				"    --prefix=[RESULT_CODE_PREFIX] | -p [RESULT_CODE_PREFIX]\n"
				"        Prefix of result code identifier.\n"
				"    --base=[BASE_VALUE] | -b [BASE_VALUE]\n"
				"        Base code value of resulted code.\n"
				"    --text | -T\n"
				"        Generate code value to text function.\n"
				"    --help | -h\n"
				"        Print help message.\n\n");
			return CRESTCODE_EMPTY_OPTIONS;
		default:
			fprintf(stderr, "unknown option code 0x%08X ??\n", c);
		}
	}
	if (optind >= argc)
	{
		fprintf(stderr, "ERROR: require code file list for scanning result code.\n");
		return CRESTCODE_ARGERR_EMPTY_SOURCE_FILES;
	}
	for (int i = optind; i < argc; i++)
	{
		char *p, *q;
		q = argv[i];
		if (NULL == (p = realpath(q, NULL)))
		{
			fprintf(stderr, "ERROR: failed at expand input path [%s]\n", q);
			perror("cannot expand given input path");
			return CRESTCODE_ARGERR_EXPAND_SRC_PATHS;
		}
		input_paths.push_back(std::string(p));
		free(p);
	}
	if (input_paths.empty())
	{
		fprintf(stderr, "ERROR: input code path is empty.\n");
		return CRESTCODE_ARGERR_EMPTY_SOURCE_FILES;
	}
	if (code_prefix.empty())
	{
		code_prefix = "ERR_";
	}
	if (base_value == 0)
	{
		base_value = 1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	std::string output_path;
	std::string code_prefix;
	std::vector<std::string> input_paths;
	int32_t base_value;
	bool gen_text_func;
	int ret_code;
	if (0 != (ret_code = parse_argv(output_path, code_prefix, input_paths, base_value, gen_text_func, argc, argv)))
	{
		return 1;
	}
	printf("INFO: output-path [%s]\n", output_path.c_str());
	printf("INFO: result-code-prefix [%s]\n", code_prefix.c_str());
	for (std::string p : input_paths)
	{
		printf("INFO: input-path [%s]\n", p.c_str());
	}
	printf("INFO: base-code-value [%d]\n", int(base_value));
	printf("INFO: generate to-text funtion [%d]\n", (gen_text_func ? 1 : 0));
	cresultcodecrawler::ResultCodes result_code_processor(base_value, code_prefix);
	if (0 != (ret_code = result_code_processor.LoadDefinitions(output_path)))
	{
		fprintf(stderr, "ERROR: load definition failed [%s]: %d.\n", output_path.c_str(), ret_code);
		return 2;
	}
	if (0 != (ret_code = result_code_processor.LoadResultCodeNames(input_paths)))
	{
		fprintf(stderr, "ERROR: load result code name failed: %d.\n", ret_code);
		return 3;
	}
	if (0 != (ret_code = result_code_processor.SaveDefinitions(output_path, gen_text_func)))
	{
		fprintf(stderr, "ERROR: save definition failed: %d.\n", ret_code);
		return 4;
	}
	result_code_processor.PrintCollectedCodes();
	return 0;
}
