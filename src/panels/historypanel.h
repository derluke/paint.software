#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>

class Document;
class QToolButton;

class HistoryPanel : public QWidget {
    Q_OBJECT
public:
    explicit HistoryPanel(QWidget *parent = nullptr);

    void setDocument(Document *doc);
    void refreshIcons();

public slots:
    void updateHistoryList();

private:
    Document *m_document = nullptr;
    bool m_navigating = false;   // guard against re-entrant history navigation
    QListWidget *m_historyList;
    QLabel *m_statusLabel;
    QToolButton *m_undoBtn = nullptr;
    QToolButton *m_redoBtn = nullptr;
};
