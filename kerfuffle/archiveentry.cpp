//
// Created by mvlabat on 5/27/16.
//

#include "archiveentry.h"

namespace Kerfuffle {
Archive::Entry::Entry(QObject *parent, QString fullPath, QString rootNode)
    : QObject(parent)
    , rootNode(rootNode)
    , compressedSizeIsSet(true)
    , m_parent(qobject_cast<Entry*>(parent))
    , m_size(0)
    , m_compressedSize(0)
    , m_isDirectory(false)
    , m_isPasswordProtected(false)
{
    if (!fullPath.isEmpty())
        setFullPath(fullPath);
}

Archive::Entry::~Entry()
{
    clear();
}

QVector<Archive::Entry*> Archive::Entry::entries()
{
    Q_ASSERT(isDir());
    return m_entries;
}

const QVector<Archive::Entry*> Archive::Entry::entries() const {
    Q_ASSERT(isDir());
    return m_entries;
}

void Archive::Entry::setEntryAt(int index, Entry *value)
{
    Q_ASSERT(isDir());
    Q_ASSERT(index < m_entries.count());
    m_entries[index] = value;
}

void Archive::Entry::appendEntry(Entry *entry)
{
    Q_ASSERT(isDir());
    m_entries.append(entry);
}

void Archive::Entry::removeEntryAt(int index)
{
    Q_ASSERT(isDir());
    Q_ASSERT(index < m_entries.count());
    delete m_entries.takeAt(index);
}

Archive::Entry *Archive::Entry::getParent() const
{
    return m_parent;
}

void Archive::Entry::setParent(Archive::Entry *parent)
{
    m_parent = parent;
}

void Archive::Entry::setFullPath(const QString &fullPath)
{
    m_fullPath = fullPath;
    const QStringList pieces = m_fullPath.split(QLatin1Char( '/' ), QString::SkipEmptyParts);
    m_name = pieces.isEmpty() ? QString() : pieces.last();
}

void Archive::Entry::setIsDirectory(const bool isDirectory)
{
    m_isDirectory = isDirectory;
}

int Archive::Entry::row() const
{
    if (getParent()) {
        return getParent()->entries().indexOf(const_cast<Archive::Entry*>(this));
    }
    return 0;
}

bool Archive::Entry::isDir() const
{
    return m_isDirectory;
}

QString Archive::Entry::name() const
{
    return m_name;
}

Archive::Entry *Archive::Entry::find(const QString & name)
{
    foreach(Entry *entry, m_entries) {
        if (entry && (entry->name() == name)) {
            return entry;
        }
    }
    return 0;
}

Archive::Entry *Archive::Entry::findByPath(const QStringList & pieces, int index)
{
    if (index == pieces.count()) {
        return 0;
    }

    Entry *next = find(pieces.at(index));

    if (index == pieces.count() - 1) {
        return next;
    }
    if (next && next->isDir()) {
        return next->findByPath(pieces, index + 1);
    }
    return 0;
}

void Archive::Entry::returnDirEntries(QList<Entry *> *store)
{
        foreach(Entry *entry, m_entries) {
            if (entry->isDir()) {
                store->prepend(entry);
                entry->returnDirEntries(store);
            }
        }
}

void Archive::Entry::clear()
{
    if (isDir()) {
        qDeleteAll(m_entries);
        m_entries.clear();
    }
}

bool Archive::Entry::operator==(const Archive::Entry &right) const
{
    return m_fullPath == right.m_fullPath;
}

QDebug operator<<(QDebug d, const Kerfuffle::Archive::Entry &entry)
{
    d.nospace() << "Entry(" << entry.property("fullPath");
    if (!entry.rootNode.isEmpty()) {
        d.nospace() << "," << entry.rootNode;
    }
    d.nospace() << ")";
    return d.space();
}

QDebug operator<<(QDebug d, const Kerfuffle::Archive::Entry *entry)
{
    d.nospace() << "Entry(" << entry->property("fullPath");
    if (!entry->rootNode.isEmpty()) {
        d.nospace() << "," << entry->rootNode;
    }
    d.nospace() << ")";
    return d.space();
}

}
