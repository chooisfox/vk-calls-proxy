#pragma once
#include <string_view>

namespace COMMON
{

constexpr std::string_view d_project_name		  = PROJECT_NAME;
constexpr std::string_view d_project_description  = PROJECT_DESCRIPTION;
constexpr std::string_view d_project_version	  = PROJECT_VERSION;
constexpr std::string_view d_application_name	  = PROJECT_APPLICATION_NAME;
constexpr std::string_view d_organization_name	  = PROJECT_ORGANIZATION_NAME;
constexpr std::string_view d_organization_website = PROJECT_ORGANIZATION_WEBSITE;
constexpr std::string_view d_developer_name		  = PROJECT_DEVELOPER_NAME;
constexpr std::string_view d_developer_email	  = PROJECT_DEVELOPER_EMAIL;

constexpr std::string_view d_compile_time	  = COMPILE_TIME;
constexpr std::string_view d_compiler_id	  = COMPILER_ID;
constexpr std::string_view d_compiler_version = COMPILER_VERSION;

constexpr std::string_view d_system_name		 = SYSTEM_NAME;
constexpr std::string_view d_system_version		 = SYSTEM_VERSION;
constexpr std::string_view d_system_id			 = SYSTEM_ID;
constexpr std::string_view d_system_processor	 = SYSTEM_PROCESSOR;
constexpr std::string_view d_system_architecture = SYSTEM_ARCHITECTURE;

constexpr std::string_view d_settings_group_generic		= "Generic";
constexpr std::string_view d_settings_group_application = "Application";
constexpr std::string_view d_settings_group_api			= "API";
constexpr std::string_view d_settings_group_utils		= "Utils";
constexpr std::string_view d_settings_group_options		= "Options";
constexpr std::string_view d_settings_group_server		= "Server";

} // namespace COMMON
