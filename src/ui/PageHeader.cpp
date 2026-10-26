// =====================================================================
//  PageHeader.cpp
// =====================================================================
#include "PageHeader.h"

#include <QVBoxLayout>

PageHeader::PageHeader(const QString &title, const QString &subtitle, QWidget *parent)
    : QWidget(parent)
{
    titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("pageTitle");

    subtitleLabel = new QLabel(subtitle, this);
    subtitleLabel->setObjectName("pageSubtitle");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
}
