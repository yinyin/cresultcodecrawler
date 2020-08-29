#ifndef _C_RESULT_CODE_CRAWLER_CODEIO_H_
#define _C_RESULT_CODE_CRAWLER_CODEIO_H_ 1

#include <cstdint>
#include <map>
#include <vector>

namespace cresultcodecrawler
{
	class ResultCodes
	{
	private:
		std::string header_guard;
		std::map<std::string, int32_t> collected_codes;

		void parseDefinitions(char *p, const char *bound);
		void parseResultCodeNames(char *p, const char *bound);

	protected:
		int32_t base_code_value;
		int32_t next_code_value;
		std::string code_prefix;

		bool isPrefixWithCodePrefix(std::string &s);
		int32_t nextCodeValue();
		void addCodeValue(std::string &code_name, int32_t code_value);
		int loadResultCodeNames(std::string &file_path);

	public:
		ResultCodes(int32_t base_code_value, std::string &code_prefix);
		~ResultCodes();

		int LoadDefinitions(std::string &file_path);
		int LoadResultCodeNames(std::vector<std::string> &file_paths);
		void PrintCollectedCodes();
	};
} // namespace cresultcodecrawler
#endif /* _C_RESULT_CODE_CRAWLER_CODEIO_H_ */
