// =====================================================================
//  SettingsPage.cpp
// =====================================================================
#include "SettingsPage.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

#include "AccountDialogs.h"
#include "PageHeader.h"
#include "Theme.h"
#include "UiHelpers.h"
#include "components/Toast.h"
#include "core/AccountRules.h"

#include <QDate>
#include <QMessageBox>

const int THEME_BUTTON_LIGHT = 0;
const int THEME_BUTTON_DARK = 1;

SettingsPage::SettingsPage(DatabaseManager &sharedDatabase, AuthService *sharedAuth, QWidget *parent)
    : Page(parent), database(sharedDatabase), auth(sharedAuth)
{
    setObjectName("settingsPage");

    QWidget *content = new QWidget(this);
    content->setObjectName("pageContent");

    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 28, 32, 32);
    contentLayout->setSpacing(20);
    contentLayout->addWidget(new PageHeader("Settings", "Your account, appearance, data and information about Finora",
                                            content));
    if (auth != nullptr) {
        contentLayout->addWidget(createProfileCard());
        contentLayout->addWidget(createSecurityCard());
        contentLayout->addWidget(createRecordsCard());
    }
    contentLayout->addWidget(createAppearanceCard());
    contentLayout->addWidget(createDataCard());
    contentLayout->addWidget(createAboutCard());
    contentLayout->addStretch();

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setObjectName("pageScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    refresh();
}

// A card with a title, a grey line under it and an empty body.
QFrame *SettingsPage::createSection(const QString &title, const QString &subtitle, QVBoxLayout *&bodyLayout)
{
    QFrame *card = new QFrame(this);
    card->setObjectName("card");
    card->setMaximumWidth(760);

    QLabel *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName("cardTitle");
    QLabel *subtitleLabel = new QLabel(subtitle, card);
    subtitleLabel->setObjectName("cardSubtitle");
    subtitleLabel->setWordWrap(true);

    bodyLayout = new QVBoxLayout();
    bodyLayout->setSpacing(12);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 18, 22, 20);
    layout->setSpacing(6);
    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addSpacing(8);
    layout->addLayout(bodyLayout);
    return card;
}

QFrame *SettingsPage::createAppearanceCard()
{
    QVBoxLayout *body = nullptr;
    QFrame *card = createSection("Appearance", "Choose how Finora looks. Your choice is saved for next time.", body);

    // Light | Dark as a segmented control.
    QFrame *themeSwitch = new QFrame(card);
    themeSwitch->setObjectName("segmented");
    QHBoxLayout *switchLayout = new QHBoxLayout(themeSwitch);
    switchLayout->setContentsMargins(3, 3, 3, 3);
    switchLayout->setSpacing(2);

    themeGroup = new QButtonGroup(card);
    QPushButton *lightButton = new QPushButton("Light", themeSwitch);
    QPushButton *darkButton = new QPushButton("Dark", themeSwitch);
    QPushButton *themeButtons[2] = {lightButton, darkButton};
    for (int i = 0; i < 2; i++) {
        themeButtons[i]->setObjectName("segmentButton");
        themeButtons[i]->setCheckable(true);
        themeButtons[i]->setCursor(Qt::PointingHandCursor);
        themeButtons[i]->setMinimumWidth(90);
        themeGroup->addButton(themeButtons[i], i);
        switchLayout->addWidget(themeButtons[i]);
    }
    lightButton->setAccessibleName("Light theme");
    darkButton->setAccessibleName("Dark theme");
    connect(themeGroup, &QButtonGroup::idClicked, this, &SettingsPage::onThemeButtonClicked);

    QLabel *themeLabel = new QLabel("Theme", card);
    themeLabel->setObjectName("bodyText");
    QHBoxLayout *themeRow = new QHBoxLayout();
    themeRow->addWidget(themeLabel);
    themeRow->addStretch();
    themeRow->addWidget(themeSwitch);

    reducedMotionBox = new QCheckBox("Reduce motion", card);
    reducedMotionBox->setObjectName("reducedMotionBox");
    QLabel *motionHelp = new QLabel("Shows changes at once, without fading messages or growing charts.", card);
    motionHelp->setObjectName("mutedText");
    motionHelp->setWordWrap(true);
    connect(reducedMotionBox, &QCheckBox::toggled, this, &SettingsPage::onReducedMotionToggled);

    body->addLayout(themeRow);
    body->addWidget(reducedMotionBox);
    body->addWidget(motionHelp);
    return card;
}

QFrame *SettingsPage::createDataCard()
{
    QVBoxLayout *body = nullptr;
    QFrame *card = createSection("Your data",
                                 "Finora keeps everything on this computer, in one SQLite database file. "
                                 "Before an upgrade changes the file, a backup copy is saved in its "
                                 "\"backups\" folder.", body);

    locationLabel = new QLabel(card);
    locationLabel->setObjectName("bodyText");
    locationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);   // the path can be copied
    locationLabel->setWordWrap(true);

    QPushButton *openButton = new QPushButton("Open data folder", card);
    openButton->setObjectName("secondaryButton");
    openButton->setCursor(Qt::PointingHandCursor);
    connect(openButton, &QPushButton::clicked, this, &SettingsPage::onOpenFolderClicked);

    body->addWidget(locationLabel);
    body->addWidget(openButton, 0, Qt::AlignLeft);
    return card;
}

QFrame *SettingsPage::createAboutCard()
{
    QVBoxLayout *body = nullptr;
    QFrame *card = createSection("About Finora", "Version " + QCoreApplication::applicationVersion()
                                 + ". A personal finance program, made as a university project.", body);

    QLabel *credits = new QLabel("Built with Qt 6, SQLite and libsodium (ISC License). Fonts: Inter and "
                                 "Manrope, both under the SIL Open Font License 1.1.", card);
    credits->setObjectName("bodyText");
    credits->setWordWrap(true);

    QPushButton *licencesButton = new QPushButton("View licences", card);
    licencesButton->setObjectName("secondaryButton");
    licencesButton->setCursor(Qt::PointingHandCursor);
    connect(licencesButton, &QPushButton::clicked, this, &SettingsPage::onLicencesClicked);

    body->addWidget(credits);
    body->addWidget(licencesButton, 0, Qt::AlignLeft);
    return card;
}

void SettingsPage::refresh()
{
    // blockSignals: showing the saved state must not count as a new choice.
    themeGroup->blockSignals(true);
    if (currentThemeMode() == THEME_DARK) {
        themeGroup->button(THEME_BUTTON_DARK)->setChecked(true);
    } else {
        themeGroup->button(THEME_BUTTON_LIGHT)->setChecked(true);
    }
    themeGroup->blockSignals(false);

    reducedMotionBox->blockSignals(true);
    reducedMotionBox->setChecked(isReducedMotion());
    reducedMotionBox->blockSignals(false);

    locationLabel->setText("Database file:  " + QDir::toNativeSeparators(database.databasePath()));

    if (auth == nullptr || !auth->session().isSignedIn()) {
        return;
    }

    // ---- The account cards ----
    UserAccount account;
    auth->signedInAccount(account);
    QString name = QString::fromStdString(account.displayName);
    profileName->setText(name);
    profileEmail->setText(QString::fromStdString(account.email));
    QDate since = QDate::fromString(QString::fromStdString(account.createdAt).left(10), "yyyy-MM-dd");
    profileSince->setText("Member since " + since.toString("d MMMM yyyy"));
    avatarLabel->setText(name.left(1).toUpper());

    int codesLeft = 0;
    auth->recoveryCodesLeft(codesLeft);
    codesLabel->setText(QString::number(codesLeft) + " of " + QString::number(RECOVERY_CODE_COUNT)
                        + " recovery codes unused. Each one resets a forgotten password once.");

    int transactions = 0;
    int budgets = 0;
    auth->countRecordsWithoutOwner(transactions, budgets);
    recordsCard->setVisible(transactions > 0 || budgets > 0);
    recordsText->setText(countText(transactions, "transaction", "transactions") + " and "
                         + countText(budgets, "budget", "budgets")
                         + " were recorded on this computer before accounts existed. "
                           "Nobody sees them until someone moves them into an account.");
}

// ---- Account cards (only while someone is signed in) ------------------------

QFrame *SettingsPage::createProfileCard()
{
    QVBoxLayout *body = nullptr;
    QFrame *card = createSection("Profile", "Who is signed in. Your records are visible only in your account.", body);

    avatarLabel = new QLabel(card);
    avatarLabel->setObjectName("avatarLarge");
    avatarLabel->setFixedSize(48, 48);
    avatarLabel->setAlignment(Qt::AlignCenter);
    profileName = new QLabel(card);
    profileName->setObjectName("accountName");
    profileEmail = new QLabel(card);
    profileEmail->setObjectName("mutedText");
    profileSince = new QLabel(card);
    profileSince->setObjectName("mutedText");

    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);
    textLayout->addWidget(profileName);
    textLayout->addWidget(profileEmail);
    textLayout->addWidget(profileSince);

    QPushButton *editButton = new QPushButton("Edit profile", card);
    editButton->setObjectName("secondaryButton");
    editButton->setCursor(Qt::PointingHandCursor);
    connect(editButton, &QPushButton::clicked, this, &SettingsPage::onEditProfileClicked);

    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(14);
    row->addWidget(avatarLabel);
    row->addLayout(textLayout, 1);
    row->addWidget(editButton, 0, Qt::AlignTop);
    body->addLayout(row);
    return card;
}

QFrame *SettingsPage::createSecurityCard()
{
    QVBoxLayout *body = nullptr;
    QFrame *card = createSection("Security",
                                 "Your password is stored only as a one-way Argon2id hash. Finora signs you out "
                                 "after " + QString::number(SESSION_IDLE_MINUTES) + " minutes without activity "
                                 "and " + QString::number(SESSION_MAX_HOURS) + " hours after signing in.", body);

    QPushButton *passwordButton = new QPushButton("Change password", card);
    passwordButton->setObjectName("secondaryButton");
    passwordButton->setCursor(Qt::PointingHandCursor);
    QLabel *passwordText = new QLabel("Password", card);
    passwordText->setObjectName("bodyText");
    QHBoxLayout *passwordRow = new QHBoxLayout();
    passwordRow->addWidget(passwordText, 1);
    passwordRow->addWidget(passwordButton);

    codesLabel = new QLabel(card);
    codesLabel->setObjectName("bodyText");
    codesLabel->setWordWrap(true);
    QPushButton *codesButton = new QPushButton("Create new codes", card);
    codesButton->setObjectName("secondaryButton");
    codesButton->setCursor(Qt::PointingHandCursor);
    QHBoxLayout *codesRow = new QHBoxLayout();
    codesRow->addWidget(codesLabel, 1);
    codesRow->addWidget(codesButton);

    QPushButton *signOutButton = new QPushButton("Sign out", card);
    signOutButton->setObjectName("dangerButton");
    signOutButton->setCursor(Qt::PointingHandCursor);

    body->addLayout(passwordRow);
    body->addLayout(codesRow);
    body->addWidget(signOutButton, 0, Qt::AlignLeft);

    connect(passwordButton, &QPushButton::clicked, this, &SettingsPage::onChangePasswordClicked);
    connect(codesButton, &QPushButton::clicked, this, &SettingsPage::onNewCodesClicked);
    connect(signOutButton, &QPushButton::clicked, this, &SettingsPage::signOutRequested);
    return card;
}

QFrame *SettingsPage::createRecordsCard()
{
    QVBoxLayout *body = nullptr;
    recordsCard = createSection("Records from before accounts",
                                "Finora never moves them on its own. Move them into your account only if they are yours.",
                                body);
    recordsText = new QLabel(recordsCard);
    recordsText->setObjectName("bodyText");
    recordsText->setWordWrap(true);
    QPushButton *moveButton = new QPushButton("Move them into my account", recordsCard);
    moveButton->setObjectName("secondaryButton");
    moveButton->setCursor(Qt::PointingHandCursor);
    connect(moveButton, &QPushButton::clicked, this, &SettingsPage::onMoveRecordsClicked);
    body->addWidget(recordsText);
    body->addWidget(moveButton, 0, Qt::AlignLeft);
    recordsCard->hide();
    return recordsCard;
}

void SettingsPage::onEditProfileClicked()
{
    EditProfileDialog dialog(*auth, this);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
        emit profileChanged();
        Toast::showMessage(this, "Profile saved.");
    }
}

void SettingsPage::onChangePasswordClicked()
{
    ChangePasswordDialog dialog(*auth, this);
    if (dialog.exec() == QDialog::Accepted) {
        Toast::showMessage(this, "Password changed.");
    }
}

void SettingsPage::onNewCodesClicked()
{
    NewRecoveryCodesDialog dialog(*auth, this);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
        Toast::showMessage(this, "New recovery codes saved. Your old codes no longer work.");
    }
}

void SettingsPage::onMoveRecordsClicked()
{
    QMessageBox::StandardButton answer = QMessageBox::question(
        this, "Move records",
        "Move all records from before accounts into your account?\n\n"
        "Only you will see them afterwards. This cannot be undone from Finora.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    int movedTransactions = 0;
    int movedBudgets = 0;
    int keptBudgets = 0;
    if (!auth->claimRecordsWithoutOwner(movedTransactions, movedBudgets, keptBudgets)) {
        QMessageBox::warning(this, "Finora", "The records could not be moved, so nothing was changed.\n\n"
                                             "Reason: " + auth->lastError());
        return;
    }
    refresh();
    Toast::showMessage(this, countText(movedTransactions, "transaction", "transactions") + " and "
                                 + countText(movedBudgets, "budget", "budgets") + " moved into your account.");
}

void SettingsPage::onThemeButtonClicked(int id)
{
    emit darkThemeChosen(id == THEME_BUTTON_DARK);
}

void SettingsPage::onReducedMotionToggled(bool checked)
{
    emit reducedMotionChosen(checked);
}

void SettingsPage::onOpenFolderClicked()
{
    QString folder = QFileInfo(database.databasePath()).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));   // opens it in File Explorer
}

// Shows the full licence texts of the fonts and of libsodium, read from
// the files that are built into Finora.exe.
void SettingsPage::onLicencesClicked()
{
    QString text;
    const int FILE_COUNT = 3;
    const char *LICENCE_FILES[FILE_COUNT] = {":/fonts/Inter-OFL.txt", ":/fonts/Manrope-OFL.txt",
                                             ":/licences/libsodium/LICENSE"};
    const char *FONT_NAMES[FILE_COUNT] = {"Inter (font)", "Manrope (font)", "libsodium 1.0.22"};
    for (int i = 0; i < FILE_COUNT; i++) {
        QFile file(LICENCE_FILES[i]);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            text += "==== " + QString(FONT_NAMES[i]) + " ====\n\n" + QString::fromUtf8(file.readAll()) + "\n\n";
            file.close();
        }
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Licences");
    dialog.resize(640, 520);

    QLabel *heading = new QLabel("Licences", &dialog);
    heading->setObjectName("dialogTitle");
    QPlainTextEdit *licenceText = new QPlainTextEdit(text, &dialog);
    licenceText->setObjectName("licenceText");
    licenceText->setReadOnly(true);
    QPushButton *closeButton = new QPushButton("Close", &dialog);
    closeButton->setObjectName("primaryButton");
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(12);
    layout->addWidget(heading);
    layout->addWidget(licenceText, 1);
    layout->addWidget(closeButton, 0, Qt::AlignRight);
    dialog.exec();
}
