set(CURRENT_LIBRARY_NAME qt6)

find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    Network
    WebSockets
    WebEngineWidgets
    WebEngineCore
    REQUIRED
)

list(APPEND PROJECT_LIBRARIES_LIST
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::WebSockets
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
)
