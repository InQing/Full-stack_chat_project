#ifndef FILEBUBBLE_H
#define FILEBUBBLE_H

#include <QWidget>
#include "BubbleFrame.h"
#include <QHBoxLayout>
#include <QLabel>

class FileBubble : public BubbleFrame
{
    Q_OBJECT
public:
    FileBubble(ChatRole role, const QString &fileName, const QString &fileSize, QWidget *parent = nullptr);

private:
    void initUI(const QString &fileName, const QString &fileSize);
    void initStyleSheet();

private:
    QLabel *m_pIconLabel;
    QLabel *m_pFileNameLabel;
    QLabel *m_pFileSizeLabel;
    QWidget *m_pContentWidget;
};

#endif // FILEBUBBLE_H
