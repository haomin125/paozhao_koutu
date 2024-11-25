#ifndef DB_UTILS_H
#define DB_UTILS_H

#include <string>

namespace db_utils
{
	class AlarmLevelCode
	{
	public:
		AlarmLevelCode() : m_nLevel(0), m_sCode("") {};
		AlarmLevelCode(int lvl, const std::string code) : m_nLevel(lvl), m_sCode(code) {};
		~AlarmLevelCode() {};

		int getLevel() { return m_nLevel; };
		std::string &getCode() { return m_sCode; };
	private:
		int m_nLevel;
		std::string m_sCode;
	};

	enum class AlarmType : char
	{
		ALARM = 'A',
		EVENT = 'E',
		USER_DEFINED = 'U'
	};

	void ALARM(const std::string& alarm);
}  //namespace db_utils

#endif // DB_UTILS_H