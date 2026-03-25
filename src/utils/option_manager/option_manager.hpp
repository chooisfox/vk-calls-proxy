#ifndef OPTION_MANAGER_HPP
#define OPTION_MANAGER_HPP

#include "manager_singleton.hpp"

#include <cxxopts.hpp>
#include <memory>
#include <string>
#include <type_traits>

namespace UTILS
{
class OptionManager : public UTILS::ManagerSingleton<OptionManager>
{
	friend class ManagerSingleton<OptionManager>;

private:
	OptionManager() = default;

	void initialize() override;

public:
	std::string_view get_manager_name() const override;

	~OptionManager();

	bool is_parsed() const;

	void		set_description(const std::string& app_name, const std::string& app_description);
	void		parse_options(int argc, char** argv);
	void		add_subcommand(const std::string& name, const std::string& description);
	bool		has_option(const std::string& name) const;
	bool		has_subcommand_option(const std::string& name);
	void		add_option(const std::string& name, const std::string& description);
	void		add_subcommand_option(const std::string& subcmd_name, const std::string& name, const std::string& description);
	size_t		get_option_count(const std::string& name) const;
	std::string get_active_subcommand() const;

	void log_help() const;
	void debug_log() const;

public:
	template<typename T>
	void add_option(const std::string& name, const std::string& description, const T& default_value);

	template<typename T>
	void add_option(const std::string& name, const std::string& description);

	template<typename T>
	void add_subcommand_option(const std::string& subcmd_name, const std::string& name, const std::string& description, const T& default_value);

	template<typename T>
	void add_subcommand_option(const std::string& subcmd_name, const std::string& name, const std::string& description);

	template<typename T = std::string>
	T get_option(const std::string& name) const;

private:
	std::unique_ptr<cxxopts::Options>	  m_options;
	std::unique_ptr<cxxopts::ParseResult> m_parsed_options;

	std::unordered_map<std::string, std::unique_ptr<cxxopts::Options>> m_subcommand_map;
	std::unique_ptr<cxxopts::ParseResult>							   m_subcmd_parsed_result;
	std::string														   m_active_subcommand;

protected:
	mutable std::mutex m_options_mutex;
};

template<typename T>
void OptionManager::add_option(const std::string& name, const std::string& description, const T& default_value)
{
	static_assert(!std::is_same_v<T, void>, "Default value required for non-void options.");
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	if (!this->m_options)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options, "Option manager not initialized");
		return;
	}

	this->m_options->add_options()(name, description, cxxopts::value<T>()->default_value(default_value));
}

template<typename T>
void OptionManager::add_option(const std::string& name, const std::string& description)
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	if (!this->m_options)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options, "Option manager not initialized");
		return;
	}

	this->m_options->add_options()(name, description, cxxopts::value<T>());
}

template<typename T>
void OptionManager::add_subcommand_option(const std::string& subcmd_name,
										  const std::string& name,
										  const std::string& description,
										  const T&			 default_value)
{
	static_assert(!std::is_same_v<T, void>, "Default value required for non-void options.");

	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	if (!this->m_options)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options, "Option manager not initialized");
		return;
	}

	auto it = m_subcommand_map.find(subcmd_name);
	if (it == m_subcommand_map.end())
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options,
						fmt::format("Cannot add option '{}': Subcommand '{}' not registered.", name, subcmd_name));
		return;
	}
	it->second->add_options()(name, description, cxxopts::value<T>()->default_value(default_value));
}

template<typename T>
void OptionManager::add_subcommand_option(const std::string& subcmd_name, const std::string& name, const std::string& description)
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	if (!this->m_options)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options, "Option manager not initialized");
		return;
	}

	auto it = m_subcommand_map.find(subcmd_name);
	if (it == m_subcommand_map.end())
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options,
						fmt::format("Cannot add option '{}': Subcommand '{}' not registered.", name, subcmd_name));
		return;
	}
	it->second->add_options()(name, description, cxxopts::value<T>());
}

template<typename T>
T OptionManager::get_option(const std::string& name) const
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	if (!this->m_options)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options, "Option manager not initialized");
		return T {};
	}

	if (!this->m_parsed_options)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options, "Options have not been parsed yet.");
		return T {};
	}

	if (m_subcmd_parsed_result && m_subcmd_parsed_result->count(name))
	{
		try
		{
			return (*m_subcmd_parsed_result)[name].as<T>();
		}
		catch (...)
		{}
	}

	if (this->m_parsed_options->count(name))
	{
		try
		{
			return (*m_parsed_options)[name].as<T>();
		}
		catch (const std::exception& e)
		{
			SPD_ERROR_CLASS(COMMON::d_settings_group_options, fmt::format("Error getting option {}: {}", name, e.what()));
		}
	}
	else
	{
		SPD_DEBUG_CLASS(COMMON::d_settings_group_options, fmt::format("Option {} not found or type mismatch. Returning default.", name));
	}

	return T {};
}
} // namespace UTILS

#endif // OPTION_MANAGER_HPP
