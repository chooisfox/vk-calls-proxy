#include "option_manager.hpp"

#include "settings_manager.hpp"
#include "spdlog_wrapper.hpp"

namespace UTILS
{

std::string_view OptionManager::get_manager_name() const
{
	return "Option Manager";
}

void OptionManager::initialize()
{
	this->m_options = std::make_unique<cxxopts::Options>(std::string(COMMON::d_project_name), std::string(COMMON::d_project_description));
	this->m_options->allow_unrecognised_options();
}

void OptionManager::set_description(const std::string& app_name, const std::string& app_description)
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	this->m_options = std::make_unique<cxxopts::Options>(app_name, app_description);
	this->m_options->allow_unrecognised_options();
}

OptionManager::~OptionManager()
{
	this->m_options.reset();
	this->m_parsed_options.reset();
	this->m_subcommand_map.clear();
	this->m_subcmd_parsed_result.reset();
}

bool OptionManager::is_parsed() const
{
	return this->m_parsed_options != nullptr;
}

void OptionManager::parse_options(int argc, char** argv)
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	if (!this->m_options)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options, "Unable to parse options, not initialized.");
		exit(1);
	}

	try
	{
		int argc_copy		   = argc;
		this->m_parsed_options = std::make_unique<cxxopts::ParseResult>(this->m_options->parse(argc_copy, argv));
		const auto& unmatched  = this->m_parsed_options->unmatched();

		if (!unmatched.empty())
		{
			std::string potential_subcmd = unmatched[0];

			auto it = m_subcommand_map.find(potential_subcmd);
			if (it != m_subcommand_map.end())
			{
				m_active_subcommand = potential_subcmd;
				SPD_INFO_CLASS(COMMON::d_settings_group_options, fmt::format("Subcommand detected: {}", m_active_subcommand));

				std::vector<const char*> sub_argv;
				sub_argv.push_back(argv[0]);

				for (size_t i = 1; i < unmatched.size(); ++i)
				{
					sub_argv.push_back(unmatched[i].c_str());
				}

				int sub_argc		   = static_cast<int>(sub_argv.size());
				m_subcmd_parsed_result = std::make_unique<cxxopts::ParseResult>(it->second->parse(sub_argc, const_cast<char**>(sub_argv.data())));
			}
			else
			{
				SPD_WARN_CLASS(COMMON::d_settings_group_options, fmt::format("Unknown subcommand or argument: {}", potential_subcmd));
			}
		}
	}
	catch (const cxxopts::exceptions::exception& e)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options, fmt::format("Error parsing options: {}", e.what()));
		exit(1);
	}
}

bool OptionManager::has_option(const std::string& name) const
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	if (!this->m_parsed_options)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options, "Options have not been parsed yet. Call parse_options() first.");
		return false;
	}

	if (m_subcmd_parsed_result && m_subcmd_parsed_result->count(name))
	{
		return true;
	}

	if (m_parsed_options && m_parsed_options->count(name))
	{
		return true;
	}

	return false;
}

bool OptionManager::has_subcommand_option(const std::string& name)
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	if (!this->m_parsed_options)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options, "Options have not been parsed yet. Call parse_options() first.");
		return false;
	}

	if (m_subcmd_parsed_result && m_subcmd_parsed_result->count(name))
	{
		return true;
	}

	return false;
}

void OptionManager::add_subcommand(const std::string& name, const std::string& description)
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	if (m_subcommand_map.find(name) != m_subcommand_map.end())
	{
		SPD_WARN_CLASS(COMMON::d_settings_group_options, fmt::format("Subcommand '{}' already exists. Overwriting.", name));
	}

	auto sub_opt = std::make_unique<cxxopts::Options>(name, description);
	sub_opt->allow_unrecognised_options();
	m_subcommand_map[name] = std::move(sub_opt);
}

void OptionManager::add_option(const std::string& name, const std::string& description)
{
	add_option<bool>(name, description);
}

void OptionManager::add_subcommand_option(const std::string& subcmd_name, const std::string& name, const std::string& description)
{
	add_subcommand_option<bool>(subcmd_name, name, description);
}

size_t OptionManager::get_option_count(const std::string& name) const
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	if (!this->m_parsed_options)
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_options, "Options have not been parsed yet. Call parseOptions() first.");
		return 0;
	}
	size_t count = 0;

	if (m_subcmd_parsed_result)
	{
		count += m_subcmd_parsed_result->count(name);
	}

	if (m_parsed_options)
	{
		count += m_parsed_options->count(name);
	}

	return count;
}

std::string OptionManager::get_active_subcommand() const
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);
	return m_active_subcommand;
}

void OptionManager::log_help() const
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	if (!m_active_subcommand.empty())
	{
		auto it = m_subcommand_map.find(m_active_subcommand);
		if (it != m_subcommand_map.end())
		{
			std::string		   help = it->second->help();
			std::istringstream iss(help);
			std::string		   line;
			while (std::getline(iss, line))
				spdlog::info(" {}", line);
			return;
		}
	}

	if (this->m_options)
	{
		std::string		   help = this->m_options->help();
		std::istringstream iss(help);
		std::string		   line;
		while (std::getline(iss, line))
			spdlog::info(" {}", line);
	}

	if (!m_subcommand_map.empty())
	{
		spdlog::info(" Available Subcommands:");
		for (const auto& pair : m_subcommand_map)
		{
			spdlog::info("   {}", pair.first);
		}
	}
}

void OptionManager::debug_log() const
{
	std::lock_guard<std::mutex> lock(this->m_options_mutex);

	auto settings_manager = UTILS::SettingsManager::instance();

	auto arguments = this->m_parsed_options->arguments();

	SPD_INFO_CLASS(COMMON::d_settings_group_options, "===========================================================");
	SPD_INFO_CLASS(COMMON::d_settings_group_options, "\tProject Information");
	SPD_INFO_CLASS(COMMON::d_settings_group_options, "-----------------------------------------------------------");
	SPD_INFO_CLASS(COMMON::d_settings_group_options, fmt::format("\tProject Name: {:<24} {}", COMMON::d_project_name, COMMON::d_project_version));
	SPD_INFO_CLASS(COMMON::d_settings_group_options, fmt::format("\tCompile Time: {}", COMMON::d_compile_time));
	SPD_INFO_CLASS(COMMON::d_settings_group_options, fmt::format("\tCompiler:     {:<24} {}", COMMON::d_compiler_id, COMMON::d_compiler_version));
	SPD_INFO_CLASS(COMMON::d_settings_group_options, "===========================================================");
	SPD_INFO_CLASS(COMMON::d_settings_group_options, "\tCommand-Line Arguments");
	SPD_INFO_CLASS(COMMON::d_settings_group_options, "-----------------------------------------------------------");
	SPD_INFO_CLASS(COMMON::d_settings_group_options, fmt::format("\tArgument Count: {}", arguments.size()));

	for (int i = 0; i < arguments.size(); ++i)
	{
		auto key_temp = arguments.at(i).key();
		auto val_temp = arguments.at(i).value();

		SPD_INFO_CLASS(COMMON::d_settings_group_options, fmt::format("\tArgument [{}]: {} = {}", i, key_temp, val_temp));
	}
	std::vector<std::string> dumped_settings;

#if defined(__cpp_lib_ranges_to_container)
	dumped_settings = settings_manager->dump() | std::ranges::views::split('\n') | std::ranges::to<std::vector<std::string>>();
#else
	std::stringstream ss(settings_manager->dump());
	std::string		  line;

	while (std::getline(ss, line, '\n'))
	{
		dumped_settings.push_back(line);
	}
#endif

	SPD_INFO_CLASS(COMMON::d_settings_group_options, "===========================================================");
	SPD_INFO_CLASS(COMMON::d_settings_group_options, "\tSettings");
	SPD_INFO_CLASS(COMMON::d_settings_group_options, "-----------------------------------------------------------");
	for (auto& line : dumped_settings)
	{
		SPD_INFO_CLASS(COMMON::d_settings_group_options, fmt::format("\t{}", line));
	}
	SPD_INFO_CLASS(COMMON::d_settings_group_options, "===========================================================");
}
} // namespace UTILS
