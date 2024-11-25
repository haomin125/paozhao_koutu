#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <fstream>
#include <mutex>

namespace file_utils
{
	//
	// save text data into /opt/history/filename
	// data format: 
	//    saveText(const std::string& data): 
	//        20200204-08:51:15.054: bot: 46, top: 248, left: 115, right: 449
	//
	//    saveText(const int prodId, const int boardId, const int viewId, const int targetId, const std::string &data):
	//        20200204-08:50:05.555: Product0-Board0-View3-Target0-Data: bot: 50, top: 262, left: 91, right: 441
	//
	class SaveTextHandler {
	public:
		static SaveTextHandler &instance(const std::string &filename);
		static void saveText(const int prodId, const int boardId, const int viewId, const int targetId, const std::string &data);
		static void saveText(const std::string &data);
	private:
		SaveTextHandler();
		virtual ~SaveTextHandler();

		void setFileName(const std::string &filename);

		static SaveTextHandler* m_instance;
		static std::mutex m_mutex;

		std::string m_sFilename;
		std::ofstream m_fstream;

		friend class Cleanup;
		class Cleanup {
		public:
			~Cleanup();
		};
	};

}  // namespase file_utils

#endif  // FILE_UTILS_H