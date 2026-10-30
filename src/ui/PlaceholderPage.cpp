// =====================================================================
//  PlaceholderPage.cpp
// =====================================================================
#include "PlaceholderPage.h"

#include <QLabel>
#include <QVBoxLayout>

#include "PageHeader.h"
#include "Theme.h"

PlaceholderPage::PlaceholderPage(const QString &title, const QString &subtitle,
                                 const QString &iconName, int milestone,
                                 const QString &plannedFeatures, QWidget *parent)
    : Page(parent), pageIconName(iconName)
{
    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(32, 28, 32, 32);
    pageLayout->setSpacing(24);

    pageLayout->addWidget(new PageHeader(title, subtitle, this));

    // The card takes all the remaining height of the page (stretch 1).
    pageLayout->addWidget(createComingSoonCard(milestone, plannedFeatures), 1);
    refresh();
}

void PlaceholderPage::refresh()
{
    iconBadge->setPixmap(tintedIcon(pageIconName, themeColors().primaryText, 30));
}

// A card with the page icon, the planned milestone and a short
// description of what the page will do.
QFrame *PlaceholderPage::createComingSoonCard(int milestone, const QString &plannedFeatures)
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");

    // A pale violet rounded square with the page's icon in the middle.
    // refresh() draws the icon in the colour of the current theme.
    iconBadge = new QLabel(card);
    iconBadge->setObjectName("emptyIcon");
    iconBadge->setFixedSize(64, 64);
    iconBadge->setAlignment(Qt::AlignCenter);

    // For example "Coming in Milestone 3". QString::number turns the int into text.
    QLabel *heading = new QLabel("Coming in Milestone " + QString::number(milestone), card);
    heading->setObjectName("emptyTitle");
    heading->setAlignment(Qt::AlignCenter);

    QLabel *details = new QLabel(plannedFeatures, card);
    details->setObjectName("emptyText");
    details->setAlignment(Qt::AlignCenter);
    details->setWordWrap(true);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(32, 32, 32, 32);
    cardLayout->setSpacing(10);
    cardLayout->addStretch();   // stretches above and below centre the content
    cardLayout->addWidget(iconBadge, 0, Qt::AlignHCenter);
    cardLayout->addSpacing(8);
    cardLayout->addWidget(heading);
    cardLayout->addWidget(details);
    cardLayout->addStretch();

    return card;
}
