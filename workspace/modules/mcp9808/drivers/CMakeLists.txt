# Custom Zephyr function that imports the mcp9808/ subdirectory if the Kconfig
# option MCP9808 is defined
add_subdirectory_ifdef(CONFIG_MCP9808 mcp9808)
