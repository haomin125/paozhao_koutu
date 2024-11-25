#ifndef DATABASE_H
#define DATABASE_H

#include "running_status.h"
#include "db_utils.h"
#include "sqlite3.h"
#include "classifier_result.h"

#include <string>
#include <vector>
#include <map>

#define DATABASE_API_OK 0

class Database {
public:
	 Database();
	 virtual ~Database();

	 struct AlarmTopStat
	 {
		 AlarmTopStat() : code(""), number("") {};

		 std::string code;
		 std::string number;

		 friend std::ostream &operator<<(std::ostream &os, const struct AlarmTopStat &t)
		 {
			 os << std::endl
				 << "code: " << t.code << ", number: " << t.number << std::endl;
			 return os;
		 }
	 };

	 struct ReportData
	 {
		 ReportData() : hhmmss(""), prod(""), lot(""), total(""), total_defect("") { defects.clear(); };

		 std::string hhmmss;
		 std::string prod;
		 std::string lot;
		 std::string total;
		 std::string total_defect;
		 std::map<std::string, std::string> defects;

		 friend std::ostream &operator<<(std::ostream &os, const struct ReportData &t)
		 {
			 os << std::endl << "hhmmss: " << t.hhmmss << ", prod: " << t.prod << ", lot: " << t.lot << ", total: " << t.total << ", total_defect: " << t.total_defect;
			 for (const auto &itr : t.defects)
			 {
				 os << ", " << itr.first << ": " << itr.second;
			 }
			 return os;
		 }
	 };

	 struct AlarmData
	 {
		 AlarmData() : uuid(""), code(""), type(""), msg(""), start_time(""), end_time(""), ack_time(""), user(""), notes("") {};

		 std::string uuid;
		 std::string code;
		 std::string type;
		 std::string msg;
		 std::string start_time;
		 std::string end_time;
		 std::string ack_time;
		 std::string user;
		 std::string notes;

		 friend std::ostream &operator<<(std::ostream &os, const struct AlarmData &t)
		 {
			 os << std::endl
				 << "uuid: " << t.uuid << ", code: " << t.code << ", type: " << t.type << ", msg: " << t.msg
				 << ", start_time: " << t.start_time << ", end_time: " << t.end_time << ", ack_time: " << t.ack_time
				 << ", user: " << t.user << ", notes: " << t.notes;
			 return os;
		 }
	 };

	 struct UserData
	 {
		 UserData() : name(""), pwd(""), role(""), pwd_time("") {};

		 std::string name;
		 std::string pwd;
		 std::string role;
		 std::string pwd_time;

		 friend std::ostream &operator<<(std::ostream &os, const struct UserData &t)
		 {
			 os << "name: " << t.name << ", pwd: " << t.pwd << ", role: " << t.role << ", pwd_time: " << t.pwd_time;
			 return os;
		 }
	 };

	 struct ApbUserData
	 {
		 ApbUserData() : userid(""), pwd(""), role(""), pwd_time(""), create_time(""), end_time(""), reminder(""), remark(std::to_string((int)remarkLevel::REMOVED)) {};

		 enum class remarkLevel : int
		 {
			 REMOVED = -1,      // user has been removed
			 NORMAL = 1         // active user
		 };

		 std::string userid;
		 std::string pwd;
		 std::string role;
		 std::string pwd_time;
		 std::string create_time;
		 std::string end_time;
		 std::string reminder;
		 std::string remark;

		 friend std::ostream &operator<<(std::ostream &os, const struct ApbUserData &t)
		 {
			 os << "userid: " << t.userid << ", pwd: " << t.pwd << ", role: " << t.role << ", pwd_time: " << t.pwd_time;
			 os << ", create_time: " << t.create_time << ", end_time: " << t.end_time << ", remind: " << t.reminder;
			 os << ", remark: " << t.remark;
			 return os;
		 }
	 };


	 void data_cleanup(const unsigned int days, const unsigned int history_keep_count);

	 int alarm_add(const std::string& code, const std::string& type, const std::string& msg, const std::string& notes = "");
	 int alarm_ack(const std::string& uuid, const std::string& usr, const std::string& action, const std::string& notes);
	 int alarm_read(std::vector<AlarmData>& amv, const int startIndex, const int endIndex, const std::string& filter, const bool hasDeleted = false);
	 int alarm_get_total(int& count, const std::string& filter, const bool hasDeleted = false);
	 int alarm_get_oneday();
	 int alarm_get_top(std::vector<AlarmTopStat>& vt, const int num, const std::string& startTime, const std::string& endTime);
	 int alarm_get_by_msg(std::vector<AlarmData>& vt, const std::string& msg, const std::string& startTime, const std::string& endTime);
	 int alarm_delete(const std::string& endTime);
	 int alarm_delete_days_ago(const int days);
	 
	 int report_add(const std::string& prod, const std::string& board, const std::string& lot, const int total, const int total_defect, const std::map<std::string, int> &defects);
	 int report_delete_by_prod(const std::string &prod);
	 int report_delete(const std::string& endTime);
	 int report_delete_days_ago(const int days);
	 int report_read(std::vector<ReportData>& rpv, const std::string& startTime, const std::string& endTime, const std::string& prod, const std::string& board);
	 int report_read_with_lot(std::vector<ReportData>& rpv, const std::string& startTime, const std::string& endTime, const std::string& prod, const std::string& board);
	 int report_read_oneday_by_hours();
	 
	 int history_add(const std::string& fileWithPath);
	 int history_total(int& count, const std::string& startTime, const std::string& endTime, const int camid, const int defectid);
	 int history_read(std::vector<std::string>& vec, const std::string& startTime, const std::string& endTime, const int camid, const int defectid, const int startIndex=0, const int endIndex=299);
	 int history_cleanup(int keepCount);

	 int prod_list(std::vector<std::string>& vt);
	 int prod_setting_write(const std::string& prod, const ProductSetting::PRODUCT_SETTING& oSetting);
	 int prod_setting_read(const std::string& prod, ProductSetting::PRODUCT_SETTING& oSetting);
	 int prod_setting_delete(const std::string &prod);
	 int production_info_write(const ProductionInfo::PRODUCTION_INFO& oProductionInfo);
	 int production_info_read(ProductionInfo::PRODUCTION_INFO& oProductionInfo);
	 int production_info_delete_by_prod(const std::string &prod);
	 int production_info_delete(const std::string& endTime);
	 int production_info_delete_days_ago(const int days);

	 int user_role(std::string& role, const std::string& user);
	 int user_list(std::vector<UserData>& usrs);
	 int user_list_by_role(std::vector<UserData>& usrs, const std::string& role);
	 int change_pwd(const std::string& usr, const std::string& new_pwd);
	 int add_user(const std::string &usr, const std::string &pwd, const std::string &role);
	 int delete_user(const std::string &usr);
	 int update_user_info(const std::string &usr, const std::string &pwd, const std::string &role);

	 int apb_user_role(std::string& role, const std::string& user);
	 int apb_user_list(std::vector<ApbUserData>& usrs, const bool all = false);
	 int apb_change_pwd(const std::string& usr, const std::string& new_pwd);
	 int add_apb_user(const std::string &usr, const std::string &pwd, const std::string &role, const std::string &end_time, const int reminder);
	 int delete_apb_user(const std::string &usr);
	 int update_apb_user_info(const std::string &usr, const std::string &pwd, const std::string &role, const std::string &end_time, const int reminder);
	 int activate_apb_user(const std::string &usr);

	 int user_privilege_add(const std::string &role, const std::string &privilege);
	 int user_privilege_update(const std::string &role, const std::string &privilege);
	 int user_privilege_delete(const std::string &role);
	 int get_user_privilege(const std::string &role, std::string &privilege);

	 bool DbOpened() { return m_bDbOpened;  }
private:
	sqlite3 *m_db;
	std::string m_sName;

	bool m_bDbOpened;
	static std::mutex m_databaseMutex;

	std::string getBindedValueSequence(const int start, const int size);
	std::string reportInsertColumns(const std::map<std::string, int> &value);
	std::string reportSelectColumns();
	std::string reportSelectColumnsWithLot();
	void bindEachDefectToReport(sqlite3_stmt *stmt, const int start, const std::map<std::string, int> &value);
};



#endif // DATABASE_H