// =====================================================================
//  PlaceholderPage.h
//  A temporary page for features that are not built yet.
// =====================================================================
#ifndef FINORA_PLACEHOLDERPAGE_H
#define FINORA_PLACEHOLDERPAGE_H

#include <QFrame>
#include <QLabel>
#include <QString>

#include "Page.h"

// Shows the page header and a card that says which milestone will add
// the feature. This way the program never pretends that a feature
// exists before it has really been built.
class PlaceholderPage : public Page
{
    Q_OBJECT

public:
    PlaceholderPage(const QString &title, const QString &subtitle,
                    const QString &iconName, int milestone,
                    const QString &plannedFeatures, QWidget *parent = nullptr);

    // Colours the page icon for the current theme (Page).
    void refresh() override;

private:
    QFrame *createComingSoonCard(int milestone, const QString &plannedFeatures);

    QString pageIconName;
    QLabel *iconBadge = nullptr;
};

#endif // FINORA_PLACEHOLDERPAGE_H
