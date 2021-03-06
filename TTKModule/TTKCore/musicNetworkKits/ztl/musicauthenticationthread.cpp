#include "musicauthenticationthread.h"

MusicAuthenticationThread::MusicAuthenticationThread(QObject *parent)
    : MusicNetworkAbstract(parent)
{
    m_reply = nullptr;
    m_manager = new QNetworkAccessManager(this);
}

MusicAuthenticationThread::~MusicAuthenticationThread()
{
    deleteAll();
}

QString MusicAuthenticationThread::getClassName()
{
    return staticMetaObject.className();
}

void MusicAuthenticationThread::deleteAll()
{
    if(m_reply)
    {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}
