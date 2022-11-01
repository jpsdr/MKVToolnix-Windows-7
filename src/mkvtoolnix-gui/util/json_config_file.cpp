#include "common/common_pch.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QtGlobal>

#include "common/qt.h"
#include "common/json.h"
#include "mkvtoolnix-gui/util/json.h"
#include "mkvtoolnix-gui/util/json_config_file.h"

namespace mtx::gui::Util {

namespace {

int
jsonIndentation() {
  static std::optional<int> s_jsonIndentation;

  if (!s_jsonIndentation)
    s_jsonIndentation = QString::fromUtf8(qgetenv("MTX_JSON_FORMAT")).toLower() == Q("indented") ? 2 : -1;

  return s_jsonIndentation.value();
}

nlohmann::json
groupToJson(JsonConfigFile::Group const &group) {
  auto json = nlohmann::json::object();

  for (auto kv = group.m_data.cbegin(), end = group.m_data.cend(); kv != end; ++kv)
    json[ to_utf8(kv.key()) ] = variantToNlohmannJson(kv.value());

  for (auto kv = group.m_groups.cbegin(), end = group.m_groups.cend(); kv != end; ++kv)
    json[ to_utf8(kv.key()) ] = groupToJson(*kv.value());

  return json;
}

JsonConfigFile::GroupPtr
jsonToGroup(nlohmann::json const &json) {
  auto group = std::make_shared<JsonConfigFile::Group>();

  if (!json.is_object())
    return group;

  for (auto kv = json.cbegin(), end = json.cend(); kv != end; ++kv) {
    auto const key   = Q(kv.key());
    auto const value = kv.value();

    if (value.is_object())
      group->m_groups.insert(key, jsonToGroup(value));

    else
      group->m_data.insert(key, nlohmannJsonToVariant(value));
  }

  return group;
}

} // anonymous namespace

// ----------------------------------------------------------------------

JsonConfigFile::JsonConfigFile(QString const &fileName)
  : ConfigFile{fileName}
{
  reset();
}

JsonConfigFile::~JsonConfigFile() {
}

void
JsonConfigFile::reset(JsonConfigFile::GroupPtr const &newGroup) {
  m_rootGroup = newGroup ? newGroup : std::make_shared<Group>();

  m_currentGroup.clear();
  m_currentGroup.push(m_rootGroup.get());
}

void
JsonConfigFile::load() {
  reset();

  QFile in{m_fileName};
  if (!in.open(QIODevice::ReadOnly))
    return;

  auto data = in.readAll();
  in.close();

  try {
    reset(jsonToGroup(mtx::json::parse(std::string{data.data(), static_cast<std::string::size_type>(data.size())})));

  } catch (std::invalid_argument const &ex) {
    qDebug() << m_fileName << "std::invalid_argument" << ex.what();
  } catch (std::exception const &ex) {
    qDebug() << m_fileName << "std::exception" << ex.what();
  } catch (...) {
    qDebug() << m_fileName << "other exception";
  }
}

void
JsonConfigFile::save() {
  QFileInfo{m_fileName}.dir().mkpath(".");
  QFile out{m_fileName};

  if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return;

  auto json = groupToJson(*m_rootGroup);
  auto dump = mtx::json::dump(json, jsonIndentation());
  out.write(QByteArray::fromRawData(dump.data(), dump.size()));
  out.flush();
  out.close();
}

void
JsonConfigFile::beginGroup(QString const &name) {
  auto &parentGroup = *m_currentGroup.top();

  if (!parentGroup.m_groups.contains(name))
    parentGroup.m_groups[name] = std::make_shared<Group>();

  m_currentGroup.push(parentGroup.m_groups[name].get());
  parentGroup.m_data.remove(name);
}

void
JsonConfigFile::endGroup() {
  if (m_currentGroup.count() > 1)
    m_currentGroup.pop();
}

void
JsonConfigFile::remove(QString const &key) {
  m_currentGroup.top()->m_data.remove(key);
}

void
JsonConfigFile::setValue(QString const &key,
                         QVariant const &value) {
  m_currentGroup.top()->m_data.insert(key, value);
}

QVariant
JsonConfigFile::value(QString const &key,
                     QVariant const &defaultValue)
  const {
  auto &data = m_currentGroup.top()->m_data;
  auto itr   = data.find(key);

  return itr == data.end() ? defaultValue : *itr;
}

QStringList
JsonConfigFile::childGroups() {
  return m_currentGroup.top()->m_groups.keys();
}

QStringList
JsonConfigFile::childKeys() {
  return m_currentGroup.top()->m_data.keys();
}

}
