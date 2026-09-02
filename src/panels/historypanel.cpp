#include "historypanel.h"
#include "../i18n.h"
#include "../toolicons.h"
#include "core/document.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QPainter>
#include <QPainterPath>

HistoryPanel::HistoryPanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    m_historyList = new QListWidget;
    m_historyList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyList->setIconSize(QSize(16, 16));
    layout->addWidget(m_historyList);

    // Barre basse annuler/rétablir
    auto *bottomBar = new QHBoxLayout;
    bottomBar->setContentsMargins(0, 0, 0, 0);
    bottomBar->addStretch();
    // Use the same painted icons as the main toolbar so baselines do not depend
    // on which glyph a user's UI font happens to provide.
    const QString btnStyle =
        "QToolButton {"
        " border: 1px solid palette(mid); border-radius: 4px; }"
        "QToolButton:hover { background: palette(alternate-base); border-color: palette(highlight); }"
        "QToolButton:pressed { background: palette(highlight); }";
    m_undoBtn = new QToolButton;
    m_undoBtn->setFixedSize(30, 26);
    m_undoBtn->setIconSize(QSize(16, 16));
    m_undoBtn->setStyleSheet(btnStyle);
    m_undoBtn->setToolTip(TR("Annuler (Ctrl+Z)"));
    m_redoBtn = new QToolButton;
    m_redoBtn->setFixedSize(30, 26);
    m_redoBtn->setIconSize(QSize(16, 16));
    m_redoBtn->setStyleSheet(btnStyle);
    m_redoBtn->setToolTip(TR("Rétablir (Ctrl+Y)"));
    refreshIcons();
    bottomBar->addWidget(m_undoBtn);
    bottomBar->addWidget(m_redoBtn);
    layout->addLayout(bottomBar);

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("color: palette(mid);");
    m_statusLabel->setVisible(false);

    connect(m_undoBtn, &QToolButton::clicked, this, [this]() {
        if (m_document) { m_document->history().undo(); }
    });
    connect(m_redoBtn, &QToolButton::clicked, this, [this]() {
        if (m_document) { m_document->history().redo(); }
    });

    // Click any entry to travel to that point in history (row 0 = Original).
    connect(m_historyList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!m_document || m_navigating) return;
        m_navigating = true;
        m_document->history().goToIndex(m_historyList->row(item));
        m_navigating = false;
    });
}

void HistoryPanel::refreshIcons() {
    if (m_undoBtn) m_undoBtn->setIcon(ToolIcons::undoAction());
    if (m_redoBtn) m_redoBtn->setIcon(ToolIcons::redoAction());
}

void HistoryPanel::setDocument(Document *doc) {
    m_document = doc;
    if (doc) {
        connect(&doc->history(), &HistoryManager::historyChanged, this, &HistoryPanel::updateHistoryList);
        updateHistoryList();
    }
}

void HistoryPanel::updateHistoryList() {
    if (!m_document) return;
    m_historyList->clear();

    auto *original = new QListWidgetItem("▶ Original");
    m_historyList->addItem(original);

    // Full timeline (done + still-redoable), so past AND future steps are shown.
    QStringList history = m_document->history().fullHistoryList();
    for (const QString &item : history) {
        m_historyList->addItem("▶ " + item);
    }

    int currentIdx = m_document->history().currentIndex();
    m_historyList->setCurrentRow(currentIdx);

    // Dim the "redo" items (those after the current position). Past/current
    // items are left with NO explicit colour so the active theme's text colour
    // applies — otherwise a hard-coded dark colour vanished on the dark theme.
    for (int i = 0; i < m_historyList->count(); ++i) {
        auto *item = m_historyList->item(i);
        if (i > currentIdx)
            item->setForeground(QColor(140, 140, 140));   // readable on light AND dark
        // else: inherit the stylesheet/theme text colour
    }
}
