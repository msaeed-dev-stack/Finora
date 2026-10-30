// =====================================================================
//  SettingsPage.h
//  Appearance (light or dark theme, reduce motion), where the data is
//  kept, and the licences of the bundled fonts.
// =====================================================================
#ifndef FINORA_SETTINGSPAGE_H
#define FINORA_SETTINGSPAGE_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "Page.h"
#include "database/DatabaseManager.h"

class SettingsPage : public Page
{
    Q_OBJECT

public:
    SettingsPage(DatabaseManager &sharedDatabase, QWidget *parent = nullptr);

    // Shows the current theme and motion setting (Page).
    void refresh() override;

signals:
    // MainWindow listens to these and changes the whole program.
    void darkThemeChosen(bool dark);
    void reducedMotionChosen(bool reduced);

private slots:
    void onThemeButtonClicked(int id);
    void onReducedMotionToggled(bool checked);
    void onOpenFolderClicked();
    void onLicencesClicked();

private:
    QFrame *createAppearanceCard();
    QFrame *createDataCard();
    QFrame *createAboutCard();
    QFrame *createSection(const QString &title, const QString &subtitle, QVBoxLayout *&bodyLayout);

    DatabaseManager &database;   // created in main(); this page only reads its location
    QButtonGroup *themeGroup = nullptr;
    QCheckBox *reducedMotionBox = nullptr;
    QLabel *locationLabel = nullptr;
};

#endif // FINORA_SETTINGSPAGE_H
