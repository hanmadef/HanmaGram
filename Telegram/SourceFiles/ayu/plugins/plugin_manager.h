// This is the source code of HanmaGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @hanmadef, 2026
#pragma once

#include "ayu/ui/boxes/plugin_info_box.h"
#include "rpl/event_stream.h"

#include <QString>
#include <vector>
#include <memory>

namespace AyuPlugins {

struct PluginEntry {
	Ui::PluginMetadata metadata;
	QString filePath;
	bool enabled = true;
};

class PluginManager {
public:
	PluginManager(const PluginManager &) = delete;
	PluginManager &operator=(const PluginManager &) = delete;

	static PluginManager &instance();

	void init();

	[[nodiscard]] QString pluginsDirectory() const;
	[[nodiscard]] const std::vector<PluginEntry> &plugins() const;

	[[nodiscard]] bool isInstalled(const QString &id) const;
	[[nodiscard]] bool isEnabled(const QString &id) const;

	bool installPlugin(const QString &sourcePath, const Ui::PluginMetadata &metadata);
	bool uninstallPlugin(const QString &id);
	void setPluginEnabled(const QString &id, bool enabled);

	void openPluginsDirectory();

	[[nodiscard]] rpl::producer<> updates() const;

private:
	PluginManager();
	void rescanPlugins();
	void saveState();
	void loadState();

	std::vector<PluginEntry> _plugins;
	rpl::event_stream<> _updates;
	QString _dir;
};

} // namespace AyuPlugins
