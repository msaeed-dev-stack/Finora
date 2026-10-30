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
#include "auth/AuthService.h"
#include "database/DatabaseManager.h"

class SettingsPage : public Page
{
    Q_OBJECT

public:
    // sharedAuth is nullptr when Finora runs without accounts (the tests of
    // Milestones 1 to 4A); then the account cards are not shown.
    SettingsPage(DatabaseManager &sharedDatabase, AuthService *sharedAuth = nullptr, QWidget *parent = nullptr);

    // Shows the current theme, motion setting and account details (Page).
    void refresh() override;

signals:
    // MainWindow listens to these and changes the whole program.
    void darkThemeChosen(bool dark);
    void reducedMotionChosen(bool reduced);
    void profileChanged();
    void signOutRequested();

private slots:
    void onThemeButtonClicked(int id);
    void onReducedMotionToggled(bool checked);
    void onOpenFolderClicked();
    void onLicencesClicked();
    void onEditProfileClicked();
    void onChangePasswordClicked();
    void onNewCodesClicked();
    void onMoveRecordsClicked();

private:
    QFrame *createProfileCard();
    QFrame *createSecurityCard();
    QFrame *createRecordsCard();
    QFrame *createAppearanceCard();
    QFrame *createDataCard();
    QFrame *createAboutCard();
    QFrame *createSection(const QString &title, const QString &subtitle, QVBoxLayout *&bodyLayout);

    DatabaseManager &database;   // created in main(); this page only reads its location
    AuthService *auth = nullptr; // nullptr without accounts
    QButtonGroup *themeGroup = nullptr;
    QLabel *avatarLabel = nullptr;
    QLabel *profileName = nullptr;
    QLabel *profileEmail = nullptr;
    QLabel *profileSince = nullptr;
    QLabel *codesLabel = nullptr;
    QFrame *recordsCard = nullptr;
    QLabel *recordsText = nullptr;
    QCheckBox *reducedMotionBox = nullptr;
    QLabel *locationLabel = nullptr;
};

#endif // FINORA_SETTINGSPAGE_H
