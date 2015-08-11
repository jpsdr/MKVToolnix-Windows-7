#include "common/common_pch.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>
#include <QStack>
#include <QtGlobal>

#include "common/qt.h"

#include "mkvtoolnix-gui/util/json_config_file.h"

namespace mtx { namespace gui { namespace Util {

static QJsonDocument::JsonFormat
jsonFormat() {
  static boost::optional<QJsonDocument::JsonFormat> s_jsonFormat;

  if (!s_jsonFormat)
    s_jsonFormat.reset( QString::fromUtf8(qgetenv("MTX_JSON_FORMAT")).toLower() == Q("indented") ? QJsonDocument::Indented : QJsonDocument::Compact );

  return s_jsonFormat.get();
}

class GroupConverter {
protected:
  QStack<QJsonObject> m_objects;

protected:
  void groupToJson(JsonConfigFile::Group &group);
  void jsonToGroup(JsonConfigFile::Group &group);

public:
  static QJsonDocument toJson(JsonConfigFile::Group &rootGroup);
  static JsonConfigFile::Group fromJson(QJsonDocument const &json);
};

void
GroupConverter::groupToJson(JsonConfigFile::Group &group) {
  for (auto kv = group.m_data.cbegin(), end = group.m_data.cend(); kv != end; ++kv)
    m_objects.top().insert(kv.key(), QJsonValue::fromVariant(kv.value()));

  for (auto kv = group.m_groups.cbegin(), end = group.m_groups.cend(); kv != end; ++kv) {
    m_objects.push({});

    groupToJson(*kv.value());

    auto newObject = m_objects.top();
    m_objects.pop();

    m_objects.top().insert(kv.key(), newObject);
  }
}

void
GroupConverter::jsonToGroup(JsonConfigFile::Group &group) {
  for (auto const &key : m_objects.top().keys()) {
    auto value = m_objects.top().value(key);

    if (value.isObject()) {
      m_objects.push(value.toObject());

      auto subGroup = std::make_shared<JsonConfigFile::Group>();
      group.m_groups.insert(key, subGroup);
      jsonToGroup(*subGroup);

      m_objects.pop();

    } else
      group.m_data.insert(key, value.toVariant());
  }
}

QJsonDocument
GroupConverter::toJson(JsonConfigFile::Group &rootGroup) {
  auto converter = GroupConverter{};

  converter.m_objects.push({});
  converter.groupToJson(rootGroup);

  auto doc = QJsonDocument{};
  doc.setObject(converter.m_objects.top());

  return doc;
}

JsonConfigFile::Group
GroupConverter::fromJson(QJsonDocument const &doc) {
  auto converter = GroupConverter{};
  auto rootGroup = JsonConfigFile::Group{};

  converter.m_objects.push(doc.object());
  converter.jsonToGroup(rootGroup);

  return rootGroup;
}

// ----------------------------------------------------------------------

JsonConfigFile::JsonConfigFile(QString const &fileName)
  : ConfigFile{fileName}
{
  clear();
}

JsonConfigFile::~JsonConfigFile() {
}

void
JsonConfigFile::clear() {
  m_rootGroup = Group{};

  m_currentGroup.clear();
  m_currentGroup.push(&m_rootGroup);
}

void
JsonConfigFile::load() {
  clear();

  QFile in{m_fileName};
  if (in.open(QIODevice::ReadOnly)) {
    auto data   = in.readAll();
    auto doc    = QJsonDocument::fromJson(data);
    m_rootGroup = GroupConverter::fromJson(doc);
  }
}

void
JsonConfigFile::save() {
  QFileInfo{m_fileName}.dir().mkpath(".");
  QFile out{m_fileName};
  if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    auto doc = GroupConverter::toJson(m_rootGroup);
    out.write(doc.toJson(jsonFormat()));
    out.close();
  }
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

}}}
