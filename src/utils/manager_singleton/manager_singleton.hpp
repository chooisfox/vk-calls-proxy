#pragma once
#include "spdlog_wrapper.hpp"

#include <memory>
#include <mutex>
#include <string_view>

namespace UTILS
{

template<typename Derived>
class ManagerSingleton
{
public:
	static std::shared_ptr<Derived> instance()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_instance)
		{
			m_instance = std::shared_ptr<Derived>(new Derived(), [](Derived* ptr) {
				delete ptr;
			});
			SPD_DEBUG_CLASS(COMMON::d_settings_group_utils, fmt::format("Initializing {}", m_instance->get_manager_name()));
			m_instance->initialize();
		}
		return m_instance;
	}

	virtual ~ManagerSingleton() = default;

	virtual std::string_view get_manager_name() const = 0;
	virtual void			 initialize()			  = 0;

protected:
	ManagerSingleton() = default;

private:
	ManagerSingleton(const ManagerSingleton&)			 = delete;
	ManagerSingleton(ManagerSingleton&&)				 = delete;
	ManagerSingleton& operator=(const ManagerSingleton&) = delete;
	ManagerSingleton& operator=(ManagerSingleton&&)		 = delete;

	static inline std::shared_ptr<Derived> m_instance;
	static inline std::mutex			   m_mutex;
};

} // namespace UTILS
