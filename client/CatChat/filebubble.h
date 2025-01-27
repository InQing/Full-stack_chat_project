#ifndef FILEBUBBLE_H
#define FILEBUBBLE_H

#include <QWidget>
#include "BubbleFrame.h"
#include <QHBoxLayout>
#include <QLabel>
#include "clickedlabel.h"

class FileBubble : public BubbleFrame
{
    Q_OBJECT
public:
    FileBubble(ChatRole role, const QString &fileName, const QString &fileSize, const QString fileId, QWidget *parent = nullptr);

private slots:
    void onDownloadClicked();

private:
    void initUI(const QString &fileName, const QString &fileSize);
    void initStyleSheet();

private:
    QLabel *m_pIconLabel;
    QLabel *m_pFileNameLabel;
    QLabel *m_pFileSizeLabel;
    QWidget *m_pContentWidget;
    ClickedLabel *m_pDownloadLabel;
    ChatRole m_role;
    const QString m_fileId;
};

#endif // FILEBUBBLE_H
