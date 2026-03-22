#ifndef SETTINGS_MANAGER_HPP
#define SETTINGS_MANAGER_HPP

#include "manager_singleton.hpp"

#include <filesystem>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <toml++/toml.hpp>
#include <vector>

namespace fs = std::filesystem;

namespace UTILS
{
class SettingsManager : public UTILS::ManagerSingleton<SettingsManager>
{
	friend class ManagerSingleton<SettingsManager>;

private:
	SettingsManager() = default;

	void initialize() override;

	void create_default_settings();

	static std::vector<std::string_view> split_path(std::string_view path);

	const toml::node*	get_node_from_table(const toml::table* table, std::string_view path) const;
	static toml::table* get_target_table(toml::table* table, const std::vector<std::string_view>& keys);

public:
	std::string_view get_manager_name() const override;

	~SettingsManager();

	bool load_settings();
	bool load_settings(fs::path file_path);
	bool save_settings();
	bool save_settings(fs::path file_path);
	bool restore_defaults();

	const toml::node* get_node(std::string_view path) const;

	template<typename T>
	T get_default_setting(std::string_view path) const;

	template<typename T>
	T get_setting(std::string_view path, T default_value) const;

	template<typename T>
	T get_setting(std::string_view path) const;

	template<typename T>
	bool set_setting(std::string_view path, T value);

	template<typename T>
	bool set_setting(std::string_view path);

	std::string dump() const;
	void		dump(std::ostream& output) const;

private:
	fs::path					 m_config_path;
	std::unique_ptr<toml::table> m_config;
	std::unique_ptr<toml::table> m_config_default;

protected:
	mutable std::mutex m_settings_mutex;
};

template<typename T>
T SettingsManager::get_default_setting(std::string_view path) const
{
	if (this->m_config_default)
	{
		if (const toml::node* node = get_node_from_table(this->m_config_default.get(), path))
		{
			std::optional<T> val = node->value<T>();
			if (val.has_value())
				return *val;
		}
	}

	return T {};
}

template<typename T>
T SettingsManager::get_setting(std::string_view path, T default_value) const
{
	std::lock_guard<std::mutex> lock(m_settings_mutex);

	if (const toml::node* node = get_node_from_table(this->m_config.get(), path))
	{
		return node->value_or(default_value);
	}

	return default_value;
}

template<typename T>
T SettingsManager::get_setting(std::string_view path) const
{
	return get_setting<T>(path, get_default_setting<T>(path));
}

template<typename T>
bool SettingsManager::set_setting(std::string_view path, T value)
{
	std::lock_guard<std::mutex> lock(m_settings_mutex);

	if (!this->m_config)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_utils, "Settings are not loaded, cannot perform set_settings.");
		return false;
	}

	std::vector<std::string_view> keys = this->split_path(path);

	if (keys.empty())
	{
		SPD_WARN_CLASS(COMMON::d_settings_group_utils, "Invalid key path provided.");
		return false;
	}

	if (toml::table* target_table = get_target_table(this->m_config.get(), keys))
	{
		target_table->insert_or_assign(keys.back(), value);
		return true;
	}

	return false;
}

template<typename T>
bool SettingsManager::set_setting(std::string_view path)
{
	return set_setting<T>(path, get_default_setting<T>(path));
}

} // namespace UTILS

#endif // SETTINGS_MANAGER_HPP
