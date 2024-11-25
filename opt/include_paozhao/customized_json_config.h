#ifndef CUSTOMIZED_JSON_CONFIG_H
#define CUSTOMIZED_JSON_CONFIG_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <vector>
#include <sstream>

#include "logger.h"

class CustomizedJsonConfig
{
public:
	static CustomizedJsonConfig &instance();

	template <typename T>
	T get(const std::string &nodeName)
	{
		// you need to try ... catch (const std::exception &e), if anything is wrong
		try
		{
			loadJson();
			return m_root.get<T>(nodeName);
		}
		catch (boost::property_tree::ptree_bad_path const &e)
		{
			LogERROR << e.what();
			throw;
		}
		catch (std::exception const &e)
		{
			LogERROR << "unexpected error, " << e.what();
			throw;
		}
	};

	template <typename T>
	std::vector<T> getVector(const std::string &nodeName)
	{
		// you need to try ... catch (const std::exception &e), if anything is wrong
		try
		{
			std::vector<T> value{};

			loadJson();
			for (const auto &itr : m_root.get_child(nodeName))
			{
				value.emplace_back(itr.second.get_value<T>());
			}
			return value;
		}
		catch (boost::property_tree::ptree_bad_path const &e)
		{
			LogERROR << e.what();
			throw;
		}
		catch (std::exception const &e)
		{
			LogERROR << "unexpected error, " << e.what();
			throw;
		}
	};

	// This method is for getting ui setting only, we need to return the json stringstream
	// under node name, please note we only support to get entire UI json stream,
	// not part of them in the middle of the tree
	std::stringstream getJsonStream(const std::string &nodeName);

	// This method is used to load json configuration from use defined file path
	void setJsonPath(const std::string &path);

private:
	CustomizedJsonConfig();
	virtual ~CustomizedJsonConfig();

	void loadJson();

	static std::mutex m_mutex;
	static CustomizedJsonConfig *m_instance;
	boost::property_tree::ptree m_root;

	std::string m_sJsonPath;
	bool m_bIsLoaded;
};

#endif