// =====================================================================
//  PlaceholderPage.h
//  A temporary page for features that are not built yet.
// =====================================================================
#ifndef FINORA_PLACEHOLDERPAGE_H
#define FINORA_PLACEHOLDERPAGE_H

#include <QFrame>
#include <QString>
#include <QWidget>

// Shows the page header and a card that says which milestone will add
// the feature. This way the program never pretends that a feature
// exists before it has really been built.
class PlaceholderPage : public QWidget
{
public:
    PlaceholderPage(const QString &title, const QString &subtitle,
                    const QString &iconName, int milestone,
                    const QString &plannedFeatures, QWidget *parent = nullptr);

private:
    QFrame *createComingSoonCard(const QString &iconName, int milestone,
                                 const QString &plannedFeatures);
};

#endif // FINORA_PLACEHOLDERPAGE_H
