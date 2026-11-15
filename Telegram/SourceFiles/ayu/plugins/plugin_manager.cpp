// This is the source code of HanmaGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @hanmadef, 2026
#include "ayu/plugins/plugin_manager.h"

#include "core/file_utilities.h"
#include "storage/localstorage.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

namespace AyuPlugins {
namespace {

constexpr auto kStateFilename = "plugins_state.json";

} // namespace

PluginManager &PluginManager::instance() {
	static PluginManager mgr;
	return mgr;
}

PluginManager::PluginManager() {
	_dir = cWorkingDir() + u"tdata/plugins/"_q;
}

QString PluginManager::pluginsDirectory() const {
	return _dir;
}

const std::vector<PluginEntry> &PluginManager::plugins() const {
	return _plugins;
}

rpl::producer<> PluginManager::updates() const {
	return _updates.events();
}

void PluginManager::init() {
	QDir().mkpath(_dir);
	rescanPlugins();
}

void PluginManager::rescanPlugins() {
	_plugins.clear();

	const auto dir = QDir(_dir);
	const auto fileList = dir.entryInfoList(
		QStringList{ u"*.plugin"_q },
		QDir::Files | QDir::Readable);

	for (const auto &info : fileList) {
		QFile file(info.absoluteFilePath());
		if (!file.open(QIODevice::ReadOnly)) {
			continue;
		}
		const auto data = file.readAll();
		file.close();

		auto metadata = Ui::ParsePluginMetadata(data);
		if (metadata.id.isEmpty() || metadata.name.isEmpty()) {
			continue;
		}

		PluginEntry entry;
		entry.metadata = std::move(metadata);
		entry.filePath = info.absoluteFilePath();
		entry.enabled = true;
		_plugins.push_back(std::move(entry));
	}

	loadState();
	_updates.fire({});
}

bool PluginManager::isInstalled(const QString &id) const {
	for (const auto &plugin : _plugins) {
		if (plugin.metadata.id.compare(id, Qt::CaseInsensitive) == 0) {
			return true;
		}
	}
	return false;
}

bool PluginManager::isEnabled(const QString &id) const {
	for (const auto &plugin : _plugins) {
		if (plugin.metadata.id.compare(id, Qt::CaseInsensitive) == 0) {
			return plugin.enabled;
		}
	}
	return false;
}

bool PluginManager::installPlugin(
		const QString &sourcePath,
		const Ui::PluginMetadata &metadata) {
	if (metadata.id.isEmpty()) {
		return false;
	}

	QDir().mkpath(_dir);
	const auto targetPath = _dir + metadata.id + u".plugin"_q;

	if (QFile::exists(targetPath)) {
		QFile::remove(targetPath);
	}

	if (!QFile::copy(sourcePath, targetPath)) {
		return false;
	}

	rescanPlugins();
	return true;
}

bool PluginManager::uninstallPlugin(const QString &id) {
	for (auto it = _plugins.begin(); it != _plugins.end(); ++it) {
		if (it->metadata.id.compare(id, Qt::CaseInsensitive) == 0) {
			QFile::remove(it->filePath);
			_plugins.erase(it);
			saveState();
			_updates.fire({});
			return true;
		}
	}
	return false;
}

void PluginManager::setPluginEnabled(const QString &id, bool enabled) {
	for (auto &plugin : _plugins) {
		if (plugin.metadata.id.compare(id, Qt::CaseInsensitive) == 0) {
			if (plugin.enabled != enabled) {
				plugin.enabled = enabled;
				saveState();
				_updates.fire({});
			}
			return;
		}
	}
}

void PluginManager::openPluginsDirectory() {
	QDir().mkpath(_dir);
	File::OpenUrl(QUrl::fromLocalFile(_dir).toString());
}

void PluginManager::saveState() {
	QJsonObject root;
	for (const auto &plugin : _plugins) {
		root[plugin.metadata.id] = plugin.enabled;
	}

	const auto path = _dir + kStateFilename;
	QFile file(path);
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
	}
}

void PluginManager::loadState() {
	const auto path = _dir + kStateFilename;
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}

	const auto doc = QJsonDocument::fromJson(file.readAll());
	if (!doc.isObject()) {
		return;
	}

	const auto root = doc.object();
	for (auto &plugin : _plugins) {
		if (root.contains(plugin.metadata.id)) {
			plugin.enabled = root.value(plugin.metadata.id).toBool(true);
		}
	}
}

} // namespace AyuPlugins
