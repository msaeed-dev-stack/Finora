// =====================================================================
//  AuditHistoryDialog.cpp
// =====================================================================
#include "AuditHistoryDialog.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <vector>

QString readableEventName(const std::string &eventType)
{
    QString name = QString::fromStdString(eventType);
    name.replace('_', ' ');
    if (!name.isEmpty()) {
        name[0] = name[0].toUpper();
    }
    // Abbreviations are written in capitals: "Csv exported" -> "CSV exported".
    name.replace("Csv", "CSV");
    name.replace("Pdf", "PDF");
    return name;
}

AuditHistoryDialog::AuditHistoryDialog(DatabaseManager &sharedDatabase, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Audit history");
    setMinimumSize(720, 480);

    QLabel *heading = new QLabel("Audit history", this);
    heading->setObjectName("dialogTitle");
    QLabel *subtitle = new QLabel("What happened in your account, newest first. Only you can see these lines; "
                                  "they are kept on this computer.", this);
    subtitle->setObjectName("mutedText");
    subtitle->setWordWrap(true);

    table = new QTableWidget(this);
    table->setObjectName("historyTable");
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"Date and time", "Event", "Details"});
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->setFrameShape(QFrame::NoFrame);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    QLabel *problem = new QLabel(this);
    problem->setObjectName("formError");
    problem->setWordWrap(true);
    problem->hide();

    std::vector<AuditEvent> events;
    if (sharedDatabase.loadAuditHistory(events)) {
        eventCount = static_cast<int>(events.size());
        table->setRowCount(eventCount);
        for (int i = 0; i < eventCount; i++) {
            QDateTime time = QDateTime::fromString(QString::fromStdString(events[i].createdAt), Qt::ISODate);
            table->setItem(i, 0, new QTableWidgetItem(time.toLocalTime().toString("d MMM yyyy, HH:mm")));
            table->setItem(i, 1, new QTableWidgetItem(readableEventName(events[i].eventType)));
            table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(events[i].details)));
        }
    } else {
        table->hide();
        problem->setText(sharedDatabase.lastError());
        problem->show();
    }

    QPushButton *closeButton = new QPushButton("Close", this);
    closeButton->setObjectName("primaryButton");
    closeButton->setDefault(true);
    connect(closeButton, &QPushButton::clicked, this, &AuditHistoryDialog::accept);
    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(closeButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(12);
    layout->addWidget(heading);
    layout->addWidget(subtitle);
    layout->addWidget(table, 1);
    layout->addWidget(problem);
    layout->addLayout(buttons);
}

int AuditHistoryDialog::shownEventCount() const
{
    return eventCount;
}
