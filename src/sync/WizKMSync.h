#ifndef WIZKMSYNC_H
#define WIZKMSYNC_H

#include <QThread>
#include <QMessageBox>

#include "WizSync.h"
#include "WizKMServer.h"

class WizDatabase;

class WizKMSyncEvents : public QObject , public IWizKMSyncEvents
{
    Q_OBJECT

    virtual void onSyncProgress(int pos);
    virtual HRESULT onText(WizKMSyncProgressMessageType type, const QString& strStatus);
    virtual HRESULT onMessage(WizKMSyncProgressMessageType type, const QString& strTitle, const QString& strMessage);
    virtual HRESULT onBubbleNotification(const QVariant& param);
    virtual void setDatabaseCount(int count);
    virtual void setCurrentDatabase(int index);
    virtual void clearLastSyncError(IWizSyncableDatabase* pDatabase);
    virtual void onTrafficLimit(IWizSyncableDatabase* pDatabase);
    virtual void onStorageLimit(IWizSyncableDatabase* pDatabase);
    virtual void onBizServiceExpr(IWizSyncableDatabase* pDatabase);
    virtual void onBizNoteCountLimit(IWizSyncableDatabase* pDatabase);
    virtual void onUploadDocument(const QString& strDocumentGUID, bool bDone);
    virtual void onBeginKb(const QString& strKbGUID);
    virtual void onEndKb(const QString& strKbGUID);

Q_SIGNALS:
    void messageReady(const QString& strStatus);
    void promptMessageRequest(int nType, const QString& strTitle, const QString& strMsg);
    void bubbleNotificationRequest(const QVariant& param);
};


class WizKMSyncThread : public QThread
{
    Q_OBJECT

public:
    WizKMSyncThread(WizDatabase& db, QObject* parent = 0);
    ~WizKMSyncThread();
    void startSyncAll(bool bBackground = true);
    bool isBackground() const;
    void stopSync();
    //
    void setFullSyncInterval(int nMinutes);
    //
    void addQuickSyncKb(const QString& kbGuid);
    //
    void quickDownloadMesages();

    bool clearCurrentToken();
    //
    void waitForDone();

public:
    static void quickSyncKb(const QString& kbGuid); //thread safe
    static bool isBusy();
    static void waitUntilIdleAndPause();
    static void setPause(bool pause);

signals:
    void startTimer(int interval);
    void stopTimer();

protected:
    virtual void run();

private slots:
    void syncAfterStart();
    void on_timerOut();

private:
    bool m_bBackground;
    WizDatabase& m_db;
    WIZUSERINFO m_info;
    WizKMSyncEvents* m_pEvents;
    bool m_bNeedSyncAll;
    bool m_bNeedDownloadMessages;
    QDateTime m_tLastSyncAll;
    int m_nFullSyncSecondsInterval;
    bool m_bBusy;
    bool m_bPause;

    //
    QMutex m_mutex;
    QWaitCondition m_wait;
    QTimer m_timer;
    std::set<QString> m_setQuickSyncKb;
    QDateTime m_tLastKbModified;

    bool doSync();

    bool prepareToken();
    bool needSyncAll();
    bool needQuickSync();
    bool needDownloadMessage();
    bool syncAll();
    bool quickSync();
    bool downloadMesages();

    void syncUserCert();
    //
    bool peekQuickSyncKb(QString& kbGuid);
    //
    friend class CWizKMSyncThreadHelper;

Q_SIGNALS:
    void syncStarted(bool syncAll);
    void syncFinished(int nErrorCode, const QString& strErrorMesssage, bool isBackground);
    void processLog(const QString& strStatus);
    void promptMessageRequest(int nType, const QString& strTitle, const QString& strMsg);
    void bubbleNotificationRequest(const QVariant& param);
};

class WizKMWaitAndPauseSyncHelper
{
public:
    WizKMWaitAndPauseSyncHelper()
    {
        WizKMSyncThread::waitUntilIdleAndPause();
    }
    ~WizKMWaitAndPauseSyncHelper()
    {
        WizKMSyncThread::setPause(false);
    }
};

#define WIZKM_WAIT_AND_PAUSE_SYNC() \
    WizKMWaitAndPauseSyncHelper __waitHelper;\
    Q_UNUSED(__waitHelper)

#define WIZKM_CHECK_SYNCING(parent) \
    if (WizKMSyncThread::isBusy()) \
    {   \
        QString title = QObject::tr("Syncing"); \
        QString message = QObject::tr("WizNote is synchronizing notes, please wait for the synchronization to complete before the operation.");  \
        QMessageBox::information(parent, title, message);\
        return;    \
    }


#endif // WIZKMSYNC_H