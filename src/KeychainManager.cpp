#include <QDebug>

#include "KeychainManager.h"

// Constants
namespace {
    const QString SERVICE_NAME = "hytromo.immersion";
}

KeychainManager::KeychainManager(QObject *parent)
    : QObject(parent),
      m_readCredentialJob(SERVICE_NAME),
      m_writeCredentialJob(SERVICE_NAME),
      m_deleteCredentialJob(SERVICE_NAME)
{
    m_readCredentialJob.setAutoDelete(false);
    m_writeCredentialJob.setAutoDelete(false);
    m_deleteCredentialJob.setAutoDelete(false);
}

void KeychainManager::readKey(const QString &key)
{
    if (key.isEmpty()) {
        emit error(tr("Cannot read key: key is empty"));
        return;
    }
    
    m_readCredentialJob.setKey(key);

    QObject::connect(&m_readCredentialJob, &QKeychain::ReadPasswordJob::finished, [this, key]() {
        if (m_readCredentialJob.error()) {
            emit error(tr("Read key failed: %1").arg(m_readCredentialJob.errorString()));
            return;
        }
        emit keyRestored(key, m_readCredentialJob.textData());
    });

    m_readCredentialJob.start();
}

void KeychainManager::writeKey(const QString &key, const QString &value)
{
    if (key.isEmpty()) {
        emit error(tr("Cannot write key: key is empty"));
        return;
    }
    
    if (value.isEmpty()) {
        emit error(tr("Cannot write key: value is empty"));
        return;
    }
    
    m_writeCredentialJob.setKey(key);
    m_writeCredentialJob.setTextData(value);

    QObject::connect(&m_writeCredentialJob, &QKeychain::WritePasswordJob::finished, [this, key]() {
        if (m_writeCredentialJob.error()) {
            emit error(tr("Write key failed: %1").arg(m_writeCredentialJob.errorString()));
            return;
        }
        emit keyStored(key);
    });

    m_writeCredentialJob.start();
}

void KeychainManager::deleteKey(const QString &key)
{
    if (key.isEmpty()) {
        emit error(tr("Cannot delete key: key is empty"));
        return;
    }
    
    m_deleteCredentialJob.setKey(key);

    QObject::connect(&m_deleteCredentialJob, &QKeychain::DeletePasswordJob::finished, [this, key]() {
        if (m_deleteCredentialJob.error()) {
            emit error(tr("Delete key failed: %1").arg(m_deleteCredentialJob.errorString()));
            return;
        }
        emit keyDeleted(key);
    });

    m_deleteCredentialJob.start();
}
