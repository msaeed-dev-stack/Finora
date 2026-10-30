// =====================================================================
//  finora_ui_tests.cpp
//  Automated checks for Finora's main window, the Transactions page and,
//  since Milestone 4, the dashboard, the Budgets page and the themes.
//
//  The tests build the real MainWindow on a real SQLite database inside
//  a temporary folder, click buttons from code, and check what appears.
//  CTest runs them with QT_QPA_PLATFORM=offscreen, so windows are drawn
//  in memory only and nothing pops up on the screen.
//
//  Optional:  finora_ui_tests --screenshots <folder>
//  saves a picture of every page and of the pop-up windows into <folder>.
// =====================================================================
#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>

#include <iostream>
#include <string>
#include <vector>

#include "auth/AuthService.h"
#include "auth/PasswordHasher.h"
#include "core/Budget.h"
#include "core/Session.h"
#include "core/Transaction.h"
#include "database/BudgetStore.h"
#include "database/DatabaseManager.h"
#include "ui/AppController.h"
#include "ui/AuthWindow.h"
#include "ui/BudgetDialog.h"
#include "ui/components/PasswordField.h"
#include "ui/components/RecoveryCodesPanel.h"
#include "ui/BudgetsPage.h"
#include "ui/DashboardPage.h"
#include "ui/MainWindow.h"
#include "ui/SettingsPage.h"
#include "ui/Theme.h"
#include "ui/TransactionDialog.h"
#include "ui/TransactionsPage.h"
#include "ui/components/MoneyFlowChart.h"
#include "ui/components/Toast.h"
#include "ui/components/WalletCard.h"

// Counts how many checks ran and how many of them failed.
struct TestCounter {
    int checksRun = 0;
    int checksFailed = 0;
};

// Records one check: prints PASS when the condition is true, FAIL otherwise.
void check(TestCounter &counter, const std::string &testName, bool passed)
{
    counter.checksRun++;

    if (passed) {
        std::cout << "PASS  " << testName << "\n";
    } else {
        counter.checksFailed++;
        std::cout << "FAIL  " << testName << "\n";
    }
}

// Counts the selected sidebar buttons. It must always be exactly 1.
int countCheckedButtons(QButtonGroup *group)
{
    int checkedCount = 0;

    for (int pageNumber = 0; pageNumber < PAGE_COUNT; pageNumber++) {
        if (group->button(pageNumber)->isChecked()) {
            checkedCount++;
        }
    }
    return checkedCount;
}

// Finds a push button by its text, for example "Edit".
QPushButton *findButton(QWidget *parent, const QString &text)
{
    QList<QPushButton *> buttons = parent->findChildren<QPushButton *>();
    for (int i = 0; i < buttons.size(); i++) {
        if (buttons[i]->text() == text) {
            return buttons[i];
        }
    }
    return nullptr;
}

// Returns the table row whose Description column shows `description`, or -1.
int findRow(QTableWidget *table, const QString &description)
{
    for (int row = 0; row < table->rowCount(); row++) {
        if (table->item(row, 3)->text() == description) {
            return row;
        }
    }
    return -1;
}

// ---- Playing the user's part in pop-up windows -----------------------
// A window opened with exec() waits for the user. Just before a test
// clicks the button that opens it, the test starts a 0 ms timer. The
// timer fires inside the pop-up's own event loop and runs one of the
// functions below, which fills in the form or clicks an answer.
//
// These two variables exist only in this test program: they pass the
// answer's text and the screenshot folder to the timer functions.
QString lastMessageText;
QString popupScreenshotFolder;

void saveScreenshot(QWidget *widget, const QString &name)
{
    if (!popupScreenshotFolder.isEmpty()) {
        widget->grab().save(popupScreenshotFolder + "/" + name + ".png");
    }
}

void answerMessageBox(QMessageBox::StandardButton answer, const QString &screenshotName)
{
    QMessageBox *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
    if (box == nullptr) {
        return;
    }
    lastMessageText = box->text();
    saveScreenshot(box, screenshotName);
    box->button(answer)->click();
}

void answerNo()
{
    answerMessageBox(QMessageBox::No, "delete_confirmation");
}

void answerYes()
{
    answerMessageBox(QMessageBox::Yes, "delete_confirmation_yes");
}

void answerOk()
{
    answerMessageBox(QMessageBox::Ok, "database_error");
}

void fillAddDialogAndSave()
{
    TransactionDialog *dialog = qobject_cast<TransactionDialog *>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
        return;
    }
    dialog->findChild<QRadioButton *>("expenseButton")->setChecked(true);
    dialog->findChild<QComboBox *>("categoryBox")->setCurrentText("Food");
    dialog->findChild<QLineEdit *>("amountEdit")->setText("1,200.50");
    QComboBox *currency = dialog->findChild<QComboBox *>("currencyBox");
    currency->setCurrentIndex(currency->findData("PKR"));
    dialog->findChild<QDateEdit *>("dateEdit")->setDate(QDate::currentDate());
    dialog->findChild<QLineEdit *>("descriptionEdit")->setText("Groceries from Ali's shop");
    saveScreenshot(dialog, "add_transaction_dialog");
    dialog->findChild<QPushButton *>("primaryButton")->click();   // Save Transaction
}

void cancelDialog()
{
    TransactionDialog *dialog = qobject_cast<TransactionDialog *>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
        return;
    }
    dialog->findChild<QLineEdit *>("amountEdit")->setText("999");
    findButton(dialog, "Cancel")->click();
}

void changeAmountAndSave()
{
    TransactionDialog *dialog = qobject_cast<TransactionDialog *>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
        return;
    }
    dialog->findChild<QLineEdit *>("amountEdit")->setText("1,500");
    dialog->findChild<QLineEdit *>("descriptionEdit")->setText("Groceries (edited)");
    saveScreenshot(dialog, "edit_transaction_dialog");
    dialog->findChild<QPushButton *>("primaryButton")->click();
}

// Builds one transaction for the tests.
Transaction makeTransaction(const std::string &type, long long amount, const std::string &currency,
                            const std::string &category, const std::string &description,
                            const QDate &date)
{
    Transaction transaction;
    transaction.type = type;
    transaction.amountMinor = amount;
    transaction.currencyCode = currency;
    transaction.category = category;
    transaction.description = description;
    transaction.date = date.toString("yyyy-MM-dd").toStdString();
    return transaction;
}

// ---- Tests -----------------------------------------------------------

// Milestone 1: the window, the sidebar and switching pages.
void testNavigation(TestCounter &counter, QApplication &app, MainWindow &window,
                    const QString &screenshotFolder)
{
    QStackedWidget *pageStack = window.findChild<QStackedWidget *>("pageStack");
    QButtonGroup *navGroup = window.findChild<QButtonGroup *>();

    check(counter, "page area exists", pageStack != nullptr);
    check(counter, "sidebar button group exists", navGroup != nullptr);
    if (pageStack == nullptr || navGroup == nullptr) {
        return;
    }

    check(counter, "window title is Finora", window.windowTitle() == "Finora");
    check(counter, "there are 7 pages", pageStack->count() == PAGE_COUNT);
    check(counter, "there are 7 sidebar buttons", navGroup->buttons().size() == PAGE_COUNT);
    check(counter, "starts on the dashboard", pageStack->currentIndex() == PAGE_DASHBOARD);
    check(counter, "dashboard button starts selected", navGroup->button(PAGE_DASHBOARD)->isChecked());

    for (int pageNumber = 0; pageNumber < PAGE_COUNT; pageNumber++) {
        QAbstractButton *button = navGroup->button(pageNumber);
        button->setFocus();   // a real mouse click also moves the keyboard focus
        button->click();
        app.processEvents();

        std::string buttonName = button->text().toStdString();
        check(counter, "clicking " + buttonName + " shows page " + std::to_string(pageNumber),
              pageStack->currentIndex() == pageNumber);
        check(counter, "only the " + buttonName + " button is selected",
              button->isChecked() && countCheckedButtons(navGroup) == 1);

        if (!screenshotFolder.isEmpty()) {
            window.grab().save(screenshotFolder + "/page_" + QString::number(pageNumber) + ".png");
        }
    }
}

// Milestone 3: the form on its own, without opening it with exec().
void testDialogValidation(TestCounter &counter)
{
    TransactionDialog dialog;
    QLineEdit *amount = dialog.findChild<QLineEdit *>("amountEdit");
    QComboBox *category = dialog.findChild<QComboBox *>("categoryBox");
    QPushButton *save = dialog.findChild<QPushButton *>("primaryButton");
    QLabel *error = dialog.findChild<QLabel *>("formError");
    QLabel *preview = dialog.findChild<QLabel *>("amountPreview");

    check(counter, "a new form is titled Add Transaction", dialog.windowTitle() == "Add Transaction");
    check(counter, "a new form starts as an expense",
          dialog.findChild<QRadioButton *>("expenseButton")->isChecked());

    amount->setText("12.345");
    save->click();
    check(counter, "Save refuses an amount with three decimals",
          dialog.result() != QDialog::Accepted && !error->isHidden()
              && error->text() == "Use at most two digits after the decimal point.");

    amount->setText("abc");
    save->click();
    check(counter, "Save refuses letters in the amount",
          dialog.result() != QDialog::Accepted
              && error->text() == "An amount may contain only digits, commas and one decimal point.");

    amount->setText("");
    save->click();
    check(counter, "Save refuses an empty amount", error->text() == "Please enter an amount.");

    amount->setText("250");
    category->setCurrentText("   ");
    save->click();
    check(counter, "Save refuses a blank category",
          dialog.result() != QDialog::Accepted && error->text() == "Category must not be empty.");

    // A valid income in USD, typed with Pakistani lakh grouping.
    dialog.findChild<QRadioButton *>("incomeButton")->setChecked(true);
    category->setCurrentText("Salary");
    QComboBox *currency = dialog.findChild<QComboBox *>("currencyBox");
    currency->setCurrentIndex(currency->findData("USD"));
    amount->setText("1,20,000");
    check(counter, "the preview shows how the amount will be saved",
          preview->text() == "Will be saved as USD 120,000.00");
    save->click();

    Transaction saved = dialog.transaction();
    check(counter, "a valid form is accepted", dialog.result() == QDialog::Accepted);
    check(counter, "the form gives the right transaction",
          saved.id == 0 && saved.type == TYPE_INCOME && saved.amountMinor == 12000000
              && saved.currencyCode == "USD" && saved.category == "Salary"
              && saved.date == QDate::currentDate().toString("yyyy-MM-dd").toStdString());

    // The edit form starts with the existing values.
    Transaction existing = makeTransaction(TYPE_INCOME, 123456, "EUR", "Freelance", "Logo design",
                                           QDate(2026, 10, 15));
    existing.id = 7;
    TransactionDialog editDialog(existing);
    check(counter, "an edit form is titled Edit Transaction", editDialog.windowTitle() == "Edit Transaction");
    check(counter, "the edit form shows the existing values",
          editDialog.findChild<QLineEdit *>("amountEdit")->text() == "1,234.56"
              && editDialog.findChild<QRadioButton *>("incomeButton")->isChecked()
              && editDialog.findChild<QComboBox *>("currencyBox")->currentData().toString() == "EUR"
              && editDialog.findChild<QComboBox *>("categoryBox")->currentText() == "Freelance"
              && editDialog.findChild<QDateEdit *>("dateEdit")->date() == QDate(2026, 10, 15)
              && editDialog.findChild<QLineEdit *>("descriptionEdit")->text() == "Logo design");
    editDialog.findChild<QPushButton *>("primaryButton")->click();
    Transaction unchanged = editDialog.transaction();
    check(counter, "saving the edit form keeps the id and the values",
          editDialog.result() == QDialog::Accepted && unchanged.id == 7 && unchanged.amountMinor == 123456
              && unchanged.date == "2026-10-15");
}

// Milestone 3: the Transactions page, used the way a person would use it.
void testTransactionsPage(TestCounter &counter, QApplication &app, MainWindow &window,
                          DatabaseManager &database, const QString &databasePath,
                          const QString &screenshotFolder)
{
    TransactionsPage *page = window.findChild<TransactionsPage *>("transactionsPage");
    check(counter, "the Transactions page is a real page, not a placeholder", page != nullptr);
    if (page == nullptr) {
        return;
    }

    QButtonGroup *navGroup = window.findChild<QButtonGroup *>();
    navGroup->button(PAGE_TRANSACTIONS)->click();
    app.processEvents();

    QTableWidget *table = page->findChild<QTableWidget *>("transactionTable");
    QLabel *emptyLabel = page->findChild<QLabel *>("emptyText");
    QLabel *countLabel = page->findChild<QLabel *>("mutedText");
    QLabel *banner = page->findChild<QLabel *>("successBanner");
    QLineEdit *search = page->findChild<QLineEdit *>("searchEdit");
    QComboBox *typeFilter = page->findChild<QComboBox *>("typeFilter");
    QComboBox *currencyFilter = page->findChild<QComboBox *>("currencyFilter");
    QComboBox *categoryFilter = page->findChild<QComboBox *>("categoryFilter");
    QComboBox *periodFilter = page->findChild<QComboBox *>("periodFilter");
    QPushButton *addButton = findButton(page, "+  Add Transaction");
    QPushButton *editButton = findButton(page, "Edit");
    QPushButton *deleteButton = findButton(page, "Delete");
    QPushButton *clearButton = findButton(page, "Clear filters");

    // 1. An empty database.
    check(counter, "an empty database shows the empty message",
          table->rowCount() == 0 && !emptyLabel->isHidden() && emptyLabel->text().startsWith("No transactions yet"));
    check(counter, "the count says 0 of 0", countLabel->text() == "Showing 0 of 0 transactions");
    check(counter, "Edit and Delete are disabled with no selection",
          !editButton->isEnabled() && !deleteButton->isEnabled());

    // 2. Adding through the real form.
    QTimer::singleShot(0, fillAddDialogAndSave);
    addButton->click();
    check(counter, "adding through the form shows one row", table->rowCount() == 1);
    check(counter, "the row shows the saved values",
          table->rowCount() == 1 && table->item(0, 1)->text() == "Expense" && table->item(0, 2)->text() == "Food"
              && table->item(0, 3)->text() == "Groceries from Ali's shop"
              && table->item(0, 4)->text() == "PKR 1,200.50");
    check(counter, "a success message is shown", !banner->isHidden() && banner->text() == "Transaction added.");
    std::vector<Transaction> stored;
    database.loadTransactions(stored);
    check(counter, "the new transaction is really in SQLite, as 120050 paisa",
          stored.size() == 1 && stored[0].amountMinor == 120050);

    // 3. Cancelling the form saves nothing.
    QTimer::singleShot(0, cancelDialog);
    addButton->click();
    check(counter, "Cancel saves nothing", table->rowCount() == 1);

    // 4. More transactions, with dates relative to today for the period filter.
    QDate today = QDate::currentDate();
    QDate firstOfThisMonth(today.year(), today.month(), 1);
    QDate firstOfLastMonth = firstOfThisMonth.addMonths(-1);
    Transaction freelance = makeTransaction(TYPE_INCOME, 500000, "USD", "Freelance", "Website for a client", firstOfLastMonth);
    Transaction bus = makeTransaction(TYPE_EXPENSE, 8000, "PKR", "Transport", "Bus fare", firstOfLastMonth.addDays(1));
    Transaction salary = makeTransaction(TYPE_INCOME, 12000000, "PKR", "Salary", "Monthly salary", firstOfThisMonth);
    Transaction coffee = makeTransaction(TYPE_EXPENSE, 450, "EUR", "Food", "Coffee", QDate(today.year() - 1, 6, 15));
    bool allSaved = page->saveNewTransaction(freelance) && page->saveNewTransaction(bus)
                    && page->saveNewTransaction(salary) && page->saveNewTransaction(coffee);
    check(counter, "four more transactions are saved", allSaved && table->rowCount() == 5);
    check(counter, "the count says 5 of 5", countLabel->text() == "Showing 5 of 5 transactions");
    check(counter, "the oldest transaction is the last row",
          table->rowCount() == 5 && table->item(4, 3)->text() == "Coffee");
    check(counter, "income amounts use the theme's income colour and expenses the expense colour",
          table->item(findRow(table, "Monthly salary"), 4)->foreground().color() == themeColors().income
              && table->item(findRow(table, "Coffee"), 4)->foreground().color() == themeColors().expenseText);
    if (!screenshotFolder.isEmpty()) {
        window.grab().save(screenshotFolder + "/transactions_all.png");
    }

    // 5. Search and filters.
    search->setText("GROC");
    check(counter, "search GROC finds Ali's groceries, ignoring case",
          table->rowCount() == 1 && table->item(0, 3)->text() == "Groceries from Ali's shop");
    search->setText("food");
    check(counter, "search food also matches the Food category: 2 rows", table->rowCount() == 2);
    if (!screenshotFolder.isEmpty()) {
        window.grab().save(screenshotFolder + "/transactions_search.png");
    }
    search->clear();

    typeFilter->setCurrentIndex(typeFilter->findData(QString::fromStdString(TYPE_INCOME)));
    check(counter, "type Income shows 2 rows", table->rowCount() == 2);
    typeFilter->setCurrentIndex(0);

    currencyFilter->setCurrentIndex(currencyFilter->findData("USD"));
    check(counter, "currency USD shows 1 row", table->rowCount() == 1);
    currencyFilter->setCurrentIndex(0);

    check(counter, "the category filter lists the 4 categories in use", categoryFilter->count() == 5);
    categoryFilter->setCurrentIndex(categoryFilter->findData("Transport"));
    check(counter, "category Transport shows 1 row", table->rowCount() == 1);
    categoryFilter->setCurrentIndex(0);

    periodFilter->setCurrentIndex(1);   // This month
    check(counter, "This month shows the 2 transactions of this month", table->rowCount() == 2);
    periodFilter->setCurrentIndex(2);   // Last month
    check(counter, "Last month shows 2 rows", table->rowCount() == 2);
    periodFilter->setCurrentIndex(3);   // This year
    int expectedThisYear = 2;
    if (firstOfLastMonth.year() == today.year()) {
        expectedThisYear = 4;   // in January, last month belongs to last year
    }
    check(counter, "This year leaves out last year's coffee", table->rowCount() == expectedThisYear);

    periodFilter->setCurrentIndex(1);
    typeFilter->setCurrentIndex(typeFilter->findData(QString::fromStdString(TYPE_EXPENSE)));
    check(counter, "This month + Expense: 1 row", table->rowCount() == 1);

    search->setText("xyz");
    check(counter, "no match shows the 'no match' message",
          table->rowCount() == 0 && !emptyLabel->isHidden()
              && emptyLabel->text() == "No transactions match these filters.");
    check(counter, "the count says 0 of 5", countLabel->text() == "Showing 0 of 5 transactions");

    clearButton->click();
    check(counter, "Clear filters shows all 5 rows again",
          table->rowCount() == 5 && search->text().isEmpty() && typeFilter->currentIndex() == 0
              && periodFilter->currentIndex() == 0);

    // 6. Editing through the real form.
    table->selectRow(findRow(table, "Groceries from Ali's shop"));
    check(counter, "selecting a row enables Edit and Delete", editButton->isEnabled() && deleteButton->isEnabled());
    QTimer::singleShot(0, changeAmountAndSave);
    editButton->click();
    int editedRow = findRow(table, "Groceries (edited)");
    check(counter, "the edited row shows the new values",
          editedRow >= 0 && table->item(editedRow, 4)->text() == "PKR 1,500.00");
    check(counter, "the update message is shown", banner->text() == "Transaction updated.");
    Transaction afterEdit;
    database.findTransactionById(stored[0].id, afterEdit);
    check(counter, "the edit is really in SQLite", afterEdit.amountMinor == 150000);

    // 7. Deleting asks first.
    table->selectRow(findRow(table, "Bus fare"));
    QTimer::singleShot(0, answerNo);
    deleteButton->click();
    check(counter, "Delete asks for confirmation", lastMessageText.startsWith("Delete this transaction?"));
    check(counter, "answering No keeps the transaction", table->rowCount() == 5 && findRow(table, "Bus fare") >= 0);

    table->selectRow(findRow(table, "Bus fare"));
    QTimer::singleShot(0, answerYes);
    deleteButton->click();
    check(counter, "answering Yes deletes it", table->rowCount() == 4 && findRow(table, "Bus fare") == -1);
    check(counter, "the delete message is shown", banner->text() == "Transaction deleted.");
    database.loadTransactions(stored);
    check(counter, "SQLite now holds 4 transactions", stored.size() == 4);
    check(counter, "Transport left the category filter", categoryFilter->findData("Transport") == -1);
    if (!screenshotFolder.isEmpty()) {
        window.grab().save(screenshotFolder + "/transactions_after_delete.png");
    }

    // 8. A database error is shown, and nothing changes.
    database.close();
    Transaction refused = makeTransaction(TYPE_EXPENSE, 100, "PKR", "Food", "While closed", today);
    QTimer::singleShot(0, answerOk);
    bool savedWhileClosed = page->saveNewTransaction(refused);
    check(counter, "saving while the database is closed fails with a warning",
          !savedWhileClosed && lastMessageText.contains("The database is not open."));
    check(counter, "the table is unchanged after the error", table->rowCount() == 4);
    bool reopened = database.open(databasePath);
    page->reloadTransactions();
    check(counter, "after reopening, the page loads the 4 rows again", reopened && table->rowCount() == 4);
}

// Milestone 3: data is still there after "restarting" Finora.
void testDataAfterRestart(TestCounter &counter, const QString &databasePath)
{
    DatabaseManager database("ui_tests_restart");
    bool opened = database.open(databasePath);
    MainWindow window(database);
    TransactionsPage *page = window.findChild<TransactionsPage *>("transactionsPage");
    QTableWidget *table = page->findChild<QTableWidget *>("transactionTable");

    check(counter, "after a restart the page shows the 4 saved transactions", opened && table->rowCount() == 4);
    int editedRow = findRow(table, "Groceries (edited)");
    check(counter, "after a restart the edited amount is still PKR 1,500.00",
          editedRow >= 0 && table->item(editedRow, 4)->text() == "PKR 1,500.00");
}

// =====================================================================
//  Milestone 4: dashboard, budgets, themes and settings
// =====================================================================

// The red message of the budget form, saved by tryDuplicateBudget().
QString lastFormError;

// Lets Qt run for a while, so that animations (growing bars, fading
// pages) and timers finish before a check or a screenshot.
void waitFor(int milliseconds)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < milliseconds) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
}

// Pages rebuild their cards with deleteLater(). This deletes the old
// cards now, so findChildren() only finds the new ones.
void flushDeletedWidgets()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

// A date in this month (monthsBack = 0) or an earlier month.
QDate dayInMonth(int monthsBack, int day)
{
    QDate month = QDate::currentDate().addMonths(-monthsBack);
    return QDate(month.year(), month.month(), day);
}

QString monthText(int monthsBack)
{
    return QDate::currentDate().addMonths(-monthsBack).toString("yyyy-MM");
}

// The message of the window's toast, or "" when no toast is showing.
QString toastText(QWidget *window)
{
    Toast *toast = window->findChild<Toast *>();
    if (toast == nullptr || toast->isHidden()) {
        return QString();
    }
    return toast->text();
}

// The visible button with this text that belongs to budget `budgetId`.
QPushButton *budgetButton(QWidget *page, const QString &text, int budgetId)
{
    QList<QPushButton *> buttons = page->findChildren<QPushButton *>();
    for (int i = 0; i < buttons.size(); i++) {
        if (buttons[i]->text() == text && buttons[i]->property("budgetId").toInt() == budgetId
            && !buttons[i]->isHidden()) {
            return buttons[i];
        }
    }
    return nullptr;
}

// The progress of the shown budget with this category and currency.
bool findShownBudget(BudgetsPage *page, const std::string &category, const std::string &currency,
                     BudgetProgress &result)
{
    const std::vector<BudgetProgress> &shown = page->shownProgress();
    for (int i = 0; i < static_cast<int>(shown.size()); i++) {
        if (shown[i].budget.category == category && shown[i].budget.currencyCode == currency) {
            result = shown[i];
            return true;
        }
    }
    return false;
}

void fillBudgetForm(BudgetDialog *dialog, const QString &category, const QString &currency, const QString &limit)
{
    dialog->findChild<QComboBox *>("budgetCategoryBox")->setCurrentText(category);
    QComboBox *currencyBox = dialog->findChild<QComboBox *>("budgetCurrencyBox");
    currencyBox->setCurrentIndex(currencyBox->findData(currency));
    dialog->findChild<QLineEdit *>("budgetLimitEdit")->setText(limit);
}

void createFoodBudget()
{
    BudgetDialog *dialog = qobject_cast<BudgetDialog *>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
        return;
    }
    fillBudgetForm(dialog, "Food", "PKR", "20,000");
    saveScreenshot(dialog, "budget_dialog");
    dialog->findChild<QPushButton *>("primaryButton")->click();   // Save Budget
}

void tryDuplicateBudget()
{
    BudgetDialog *dialog = qobject_cast<BudgetDialog *>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
        return;
    }
    fillBudgetForm(dialog, "food", "PKR", "100");
    dialog->findChild<QPushButton *>("primaryButton")->click();   // refused: stays open
    lastFormError = dialog->findChild<QLabel *>("formError")->text();
    saveScreenshot(dialog, "budget_dialog_duplicate");
    findButton(dialog, "Cancel")->click();
}

void changeBudgetLimit()
{
    BudgetDialog *dialog = qobject_cast<BudgetDialog *>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
        return;
    }
    dialog->findChild<QLineEdit *>("budgetLimitEdit")->setText("25,000");
    dialog->findChild<QPushButton *>("primaryButton")->click();
}

void answerBudgetNo()
{
    answerMessageBox(QMessageBox::No, "budget_delete_confirmation");
}

void answerBudgetYes()
{
    answerMessageBox(QMessageBox::Yes, "budget_delete_confirmation_yes");
}

// Saves a picture of the open pop-up form under `nextDialogPicture`,
// then closes it with Cancel. Used to show the forms in the dark theme.
QString nextDialogPicture;

void pictureDialogAndCancel()
{
    QDialog *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
        return;
    }
    saveScreenshot(dialog, nextDialogPicture);
    dialog->reject();
}

void saveWindowScreenshot(QWidget &window, const QString &folder, const QString &name)
{
    if (!folder.isEmpty()) {
        waitFor(700);   // let the bars, rings and page fade finish
        window.grab().save(folder + "/" + name + ".png");
    }
}

void testDashboardAndBudgets(TestCounter &counter, QApplication &app, const QString &databasePath,
                             const QString &screenshotFolder)
{
    DatabaseManager database("ui_tests_m4");
    bool opened = database.open(databasePath);
    check(counter, "the Milestone 4 test database opens", opened);
    if (!opened) {
        return;
    }

    // Known transactions: this month (0) and last month (1).
    Transaction rows[8] = {
        makeTransaction(TYPE_INCOME, 12000000, "PKR", "Salary", "October salary", dayInMonth(0, 1)),
        makeTransaction(TYPE_EXPENSE, 1650000, "PKR", "Food", "Groceries for the month", dayInMonth(0, 2)),
        makeTransaction(TYPE_EXPENSE, 158050, "PKR", "Food", "Dinner with family", dayInMonth(0, 3)),
        makeTransaction(TYPE_EXPENSE, 500000, "PKR", "Transport", "Fuel", dayInMonth(0, 4)),
        makeTransaction(TYPE_INCOME, 50000, "USD", "Freelance", "Logo design", dayInMonth(0, 5)),
        makeTransaction(TYPE_EXPENSE, 13500, "USD", "Shopping", "Headphones", dayInMonth(0, 6)),
        makeTransaction(TYPE_EXPENSE, 4500000, "PKR", "Rent", "Flat rent", dayInMonth(1, 1)),
        makeTransaction(TYPE_INCOME, 20000, "EUR", "Gift", "Birthday gift", dayInMonth(1, 15))
    };
    bool allSaved = true;
    for (int i = 0; i < 8; i++) {
        allSaved = database.addTransaction(rows[i]) && allSaved;
    }
    check(counter, "8 known transactions are saved", allSaved);

    MainWindow window(database);
    window.resize(1280, 900);
    window.show();
    app.processEvents();

    // ---- Dashboard: wallet cards ----
    DashboardPage *dashboard = window.findChild<DashboardPage *>("dashboardPage");
    check(counter, "the dashboard is a real page now", dashboard != nullptr);
    if (dashboard == nullptr) {
        return;
    }
    WalletCard *pkr = dashboard->walletCard("PKR");
    WalletCard *usd = dashboard->walletCard("USD");
    WalletCard *eur = dashboard->walletCard("EUR");
    check(counter, "there is one wallet card for each currency", pkr && usd && eur);
    check(counter, "PKR card: the balance over all time is PKR 51,919.50", pkr->balanceText() == "PKR 51,919.50");
    check(counter, "PKR card: this month's income and spending",
          pkr->incomeText() == "PKR 120,000.00" && pkr->expenseText() == "PKR 23,080.50");
    check(counter, "USD card: added up on its own, balance USD 365.00",
          usd->balanceText() == "USD 365.00" && usd->expenseText() == "USD 135.00");
    check(counter, "EUR card: last month's gift is in the balance but not in this month",
          eur->balanceText() == "EUR 200.00" && eur->incomeText() == "EUR 0.00");

    // ---- Dashboard: the chart ----
    MoneyFlowChart *chart = dashboard->chart();
    std::vector<MonthFlow> flows = chart->flows();
    check(counter, "the chart shows 6 months of PKR", flows.size() == 6 && dashboard->chartCurrency() == "PKR");
    check(counter, "the newest bars are this month's PKR income and spending",
          flows.size() == 6 && flows[5].incomeMinor == 12000000 && flows[5].expenseMinor == 2308050);
    check(counter, "last month's rent is in the bar before", flows.size() == 6 && flows[4].expenseMinor == 4500000);

    dashboard->setChartCurrency("USD");
    flows = chart->flows();
    check(counter, "switching the chart to USD shows only dollars",
          flows.size() == 6 && flows[5].incomeMinor == 50000 && flows[5].expenseMinor == 13500);
    findButton(dashboard, "12 months")->click();
    check(counter, "the 12 months button shows 12 bars", chart->flows().size() == 12);
    findButton(dashboard, "6 months")->click();
    dashboard->setChartCurrency("PKR");

    // Keyboard: End highlights the newest month, Left the month before.
    chart->setFocus();
    QKeyEvent endKey(QEvent::KeyPress, Qt::Key_End, Qt::NoModifier);
    QApplication::sendEvent(chart, &endKey);
    QKeyEvent leftKey(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
    QApplication::sendEvent(chart, &leftKey);
    check(counter, "the arrow keys move the highlight to last month", chart->highlightedIndex() == 4);
    check(counter, "the chart describes the highlighted month in words",
          chart->summaryText().contains("spent PKR 45,000.00"));
    chart->setHighlightedIndex(-1);

    QTableWidget *recent = dashboard->findChild<QTableWidget *>("recentTable");
    check(counter, "recent transactions shows the 6 newest of 8", recent->rowCount() == 6);
    check(counter, "spending has a minus sign and income a plus sign",
          recent->item(0, 3)->text() == "-USD 135.00" && recent->item(5, 3)->text() == "+PKR 120,000.00");
    check(counter, "with no budgets there are no budget alerts", dashboard->budgetAlertCount() == 0);
    saveWindowScreenshot(window, screenshotFolder, "m4_dashboard_no_budgets");

    // ---- Budgets page ----
    QButtonGroup *navGroup = window.findChild<QButtonGroup *>("navButtonGroup");
    navGroup->button(PAGE_BUDGETS)->click();
    BudgetsPage *budgetsPage = window.findChild<BudgetsPage *>("budgetsPage");
    check(counter, "the Budgets page is a real page now", budgetsPage != nullptr);
    if (budgetsPage == nullptr) {
        return;
    }
    check(counter, "with no budgets the page shows the empty state",
          budgetsPage->shownProgress().empty() && findButton(budgetsPage, "Create your first budget") != nullptr
              && findButton(budgetsPage, "Create your first budget")->isVisible());
    saveWindowScreenshot(window, screenshotFolder, "m4_budgets_empty");

    // Create a budget with the real button and the real form.
    QTimer::singleShot(0, createFoodBudget);
    findButton(budgetsPage, "+  New Budget")->click();
    flushDeletedWidgets();
    BudgetProgress food;
    bool foodShown = findShownBudget(budgetsPage, "Food", "PKR", food);
    check(counter, "the form created a Food budget of PKR 20,000.00",
          foodShown && food.budget.limitMinor == 2000000 && food.budget.id > 0);
    check(counter, "Food at 90% (18,080.50 of 20,000) is 'Nearing limit'",
          food.percentUsed == 90 && food.status == BUDGET_WARNING);
    check(counter, "a toast confirms the new budget", toastText(&window) == "Budget created.");

    // The form refuses a second Food budget in PKR and stays open.
    lastFormError.clear();
    QTimer::singleShot(0, tryDuplicateBudget);
    findButton(budgetsPage, "+  New Budget")->click();
    check(counter, "the form refuses a second Food budget in PKR, also in lower case",
          lastFormError.startsWith("There is already a Food budget in PKR"));
    check(counter, "the refused form saved nothing", budgetsPage->shownProgress().size() == 1);

    Budget transport;
    transport.category = "Transport";
    transport.currencyCode = "PKR";
    transport.limitMinor = 500000;   // exactly what was spent
    Budget shopping;
    shopping.category = "Shopping";
    shopping.currencyCode = "USD";
    shopping.limitMinor = 10000;     // 135.00 spent of 100.00
    Budget rent;
    rent.category = "Rent";
    rent.currencyCode = "PKR";
    rent.limitMinor = 5000000;       // no rent this month
    bool threeSaved = budgetsPage->saveNewBudget(transport) && budgetsPage->saveNewBudget(shopping)
                      && budgetsPage->saveNewBudget(rent);
    flushDeletedWidgets();
    check(counter, "three more budgets are saved", threeSaved && budgetsPage->shownProgress().size() == 4);

    BudgetProgress shown;
    check(counter, "Transport spent exactly its limit: 'Limit reached'",
          findShownBudget(budgetsPage, "Transport", "PKR", shown) && shown.status == BUDGET_REACHED
              && shown.remainingMinor == 0);
    check(counter, "Shopping in USD is over budget by USD 35.00",
          findShownBudget(budgetsPage, "Shopping", "USD", shown) && shown.status == BUDGET_OVER
              && shown.remainingMinor == -3500 && shown.percentUsed == 135);
    check(counter, "Rent has nothing spent this month: 'On track'",
          findShownBudget(budgetsPage, "Rent", "PKR", shown) && shown.status == BUDGET_ON_TRACK);
    check(counter, "every budget card has its own Edit and Delete buttons",
          budgetButton(budgetsPage, "Edit", rent.id) != nullptr && budgetButton(budgetsPage, "Delete", rent.id) != nullptr);
    saveWindowScreenshot(window, screenshotFolder, "m4_budgets_light");

    // Last month: rent was paid, so Rent is at 90%; Food was not used.
    budgetsPage->monthSwitcher()->setMonth(monthText(1));
    check(counter, "last month Rent was at 90%",
          findShownBudget(budgetsPage, "Rent", "PKR", shown) && shown.percentUsed == 90
              && shown.status == BUDGET_WARNING);
    check(counter, "last month nothing was spent on Food",
          findShownBudget(budgetsPage, "Food", "PKR", shown) && shown.spentMinor == 0);
    budgetsPage->monthSwitcher()->setMonth(monthText(0));
    flushDeletedWidgets();

    // Edit Food to PKR 25,000.00 with its own Edit button.
    QTimer::singleShot(0, changeBudgetLimit);
    budgetButton(budgetsPage, "Edit", food.budget.id)->click();
    flushDeletedWidgets();
    check(counter, "after editing, Food is PKR 25,000.00 and on track (72%)",
          findShownBudget(budgetsPage, "Food", "PKR", shown) && shown.budget.limitMinor == 2500000
              && shown.percentUsed == 72 && shown.status == BUDGET_ON_TRACK);

    // Delete Rent: No keeps it, Yes deletes it.
    QTimer::singleShot(0, answerBudgetNo);
    budgetButton(budgetsPage, "Delete", rent.id)->click();
    flushDeletedWidgets();
    check(counter, "deleting a budget asks first",
          lastMessageText.startsWith("Delete the budget Rent"));
    check(counter, "answering No keeps the budget", budgetsPage->shownProgress().size() == 4);
    QTimer::singleShot(0, answerBudgetYes);
    budgetButton(budgetsPage, "Delete", rent.id)->click();
    flushDeletedWidgets();
    BudgetStore checkStore(database);
    std::vector<Budget> stored;
    checkStore.loadBudgets(stored);
    check(counter, "answering Yes deletes it from SQLite", budgetsPage->shownProgress().size() == 3 && stored.size() == 3);
    check(counter, "a toast confirms the deletion", toastText(&window) == "Budget deleted.");

    // ---- Back on the dashboard ----
    navGroup->button(PAGE_DASHBOARD)->click();
    check(counter, "the dashboard now lists 2 budget alerts (Transport reached, Shopping over)",
          dashboard->budgetAlertCount() == 2);
    saveWindowScreenshot(window, screenshotFolder, "m4_dashboard_light");

    // "+ Add Transaction" on the dashboard uses the same form as the Transactions page.
    QTimer::singleShot(0, fillAddDialogAndSave);   // PKR 1,200.50 Food, today
    findButton(dashboard, "+  Add Transaction")->click();
    check(counter, "a transaction added on the dashboard updates the PKR card at once",
          pkr->expenseText() == "PKR 24,281.00");
    check(counter, "a toast confirms the new transaction", toastText(&window) == "Transaction added.");

    // ---- Theme switching ----
    QPushButton *themeToggle = window.findChild<QPushButton *>("themeToggle");
    check(counter, "Finora starts in the light theme", !window.isDarkTheme() && themeToggle->text() == "Dark mode");
    themeToggle->click();
    check(counter, "the sidebar button switches to the dark theme",
          window.isDarkTheme() && app.styleSheet().contains(darkThemeColors().canvas.name()));
    check(counter, "the dark theme is saved in the database", database.setting("theme", "") == "dark");
    check(counter, "the button now offers the light theme", themeToggle->text() == "Light mode");
    check(counter, "a toast says the dark theme is on", toastText(&window) == "Dark theme on");
    // Find the USD 500.00 income by its text: its row depends on today's
    // date, because the groceries added above are dated today.
    int incomeRow = -1;
    for (int row = 0; row < recent->rowCount(); row++) {
        if (recent->item(row, 3)->text() == "+USD 500.00") {
            incomeRow = row;
        }
    }
    check(counter, "amounts are drawn in the dark theme's colours",
          incomeRow >= 0 && recent->item(incomeRow, 3)->foreground().color() == darkThemeColors().income);
    saveWindowScreenshot(window, screenshotFolder, "m4_dashboard_dark");

    // Pictures of both forms in the dark theme; Cancel saves nothing.
    nextDialogPicture = "transaction_dialog_dark";
    QTimer::singleShot(0, pictureDialogAndCancel);
    findButton(dashboard, "+  Add Transaction")->click();
    check(counter, "cancelling the form on the dashboard adds nothing", pkr->expenseText() == "PKR 24,281.00");

    navGroup->button(PAGE_BUDGETS)->click();
    nextDialogPicture = "budget_dialog_dark";
    QTimer::singleShot(0, pictureDialogAndCancel);
    findButton(budgetsPage, "+  New Budget")->click();
    flushDeletedWidgets();
    check(counter, "cancelling the budget form adds nothing", budgetsPage->shownProgress().size() == 3);
    saveWindowScreenshot(window, screenshotFolder, "m4_budgets_dark");
    navGroup->button(PAGE_TRANSACTIONS)->click();
    saveWindowScreenshot(window, screenshotFolder, "m4_transactions_dark");
    navGroup->button(PAGE_GOALS)->click();
    saveWindowScreenshot(window, screenshotFolder, "m4_goals_dark");

    // ---- Settings ----
    navGroup->button(PAGE_SETTINGS)->click();
    SettingsPage *settings = window.findChild<SettingsPage *>("settingsPage");
    QCheckBox *motionBox = settings->findChild<QCheckBox *>("reducedMotionBox");
    check(counter, "Settings shows the dark theme as selected", findButton(settings, "Dark")->isChecked());
    saveWindowScreenshot(window, screenshotFolder, "m4_settings_dark");

    motionBox->click();
    check(counter, "Reduce motion turns animations off and is saved",
          isReducedMotion() && database.setting("reduced_motion", "0") == "1");
    motionBox->click();
    check(counter, "turning it off again is saved too",
          !isReducedMotion() && database.setting("reduced_motion", "1") == "0");

    findButton(settings, "Light")->click();
    check(counter, "the Light button in Settings switches back and saves it",
          !window.isDarkTheme() && database.setting("theme", "") == "light"
              && app.styleSheet().contains(lightThemeColors().canvas.name()));
    saveWindowScreenshot(window, screenshotFolder, "m4_settings_light");
    navGroup->button(PAGE_TRANSACTIONS)->click();
    saveWindowScreenshot(window, screenshotFolder, "m4_transactions_light");
}

// The theme is remembered when Finora starts again.
void testAppearanceAfterRestart(TestCounter &counter, const QString &databasePath)
{
    DatabaseManager database("ui_tests_m4_restart");
    database.open(databasePath);
    database.saveSetting("theme", "dark");
    database.saveSetting("reduced_motion", "1");
    {
        MainWindow window(database);
        check(counter, "after a restart Finora opens in the saved dark theme, with reduced motion",
              window.isDarkTheme() && isReducedMotion());
    }
    database.saveSetting("theme", "light");
    database.saveSetting("reduced_motion", "0");
    MainWindow window(database);
    check(counter, "and in the light theme once light is saved", !window.isDarkTheme() && !isReducedMotion());
}

// =====================================================================
//  Milestone 4, Phase 4B: the sign-in window and accounts
// =====================================================================

// Answers the "Records from before accounts" question with "Move them
// into my account" (started with a 0 ms timer, like the other pop-ups).
void acceptClaimQuestion()
{
    QMessageBox *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
    if (box == nullptr) {
        return;
    }
    lastMessageText = box->text();
    saveScreenshot(box, "claim_question");
    findButton(box, "Move them into my account")->click();
}

void typePassword(QWidget *window, const QString &fieldName, const QString &text)
{
    window->findChild<PasswordField *>(fieldName)->lineEdit()->setText(text);
}

void typeText(QWidget *window, const QString &fieldName, const QString &text)
{
    window->findChild<QLineEdit *>(fieldName)->setText(text);
}

// The text of the first label with this object name that is not hidden.
QString shownLabelText(QWidget *window, const QString &objectName)
{
    QList<QLabel *> labels = window->findChildren<QLabel *>(objectName);
    for (int i = 0; i < labels.size(); i++) {
        if (!labels[i]->isHidden()) {
            return labels[i]->text();
        }
    }
    return QString();
}

bool someLabelContains(QWidget *window, const QString &text)
{
    QList<QLabel *> labels = window->findChildren<QLabel *>();
    for (int i = 0; i < labels.size(); i++) {
        if (labels[i]->text().contains(text)) {
            return true;
        }
    }
    return false;
}

// Signs up through the real form and returns the shown recovery codes.
QStringList signUpThroughForm(QApplication &app, AuthWindow *authWindow, const QString &name,
                              const QString &email, const QString &password)
{
    typeText(authWindow, "signUpName", name);
    typeText(authWindow, "signUpEmail", email);
    typePassword(authWindow, "signUpPassword", password);
    typePassword(authWindow, "signUpConfirm", password);
    findButton(authWindow, "Create account")->click();
    app.processEvents();
    return authWindow->findChild<RecoveryCodesPanel *>()->shownCodes();
}

void signInThroughForm(QApplication &app, AuthWindow *authWindow, const QString &email, const QString &password)
{
    typeText(authWindow, "signInEmail", email);
    typePassword(authWindow, "signInPassword", password);
    findButton(authWindow, "Sign in")->click();
    app.processEvents();
}

void testAccountsWindow(TestCounter &counter, QApplication &app, const QString &folder,
                        const QString &screenshotFolder)
{
    // One transaction recorded before accounts existed.
    QString path = folder + "/ui_accounts.db";
    {
        DatabaseManager before("ui_accounts_before");
        before.open(path);
        Transaction groceries = makeTransaction(TYPE_EXPENSE, 250000, "PKR", "Food", "Groceries before accounts",
                                                QDate::currentDate());
        before.addTransaction(groceries);
    }

    Session session;
    DatabaseManager database("ui_accounts");
    database.open(path);
    database.attachSession(session);
    PasswordHasher fast(HASH_INTERACTIVE);   // fast for the tests; Finora uses MODERATE
    AuthService auth(database, session, fast, fast);
    AppController controller(database, auth, 600);
    controller.start();
    app.processEvents();
    AuthWindow *authWindow = controller.authWindow();

    // ---- Welcome and sign-up ----
    check(counter, "Finora starts with the sign-in window, not the main window",
          authWindow->isVisible() && controller.mainWindow() == nullptr);
    check(counter, "with no account yet it shows the welcome page", authWindow->currentPage() == AuthWindow::PAGE_WELCOME);
    check(counter, "the window says these are local demonstration accounts",
          someLabelContains(authWindow, "Local demonstration accounts"));
    if (!screenshotFolder.isEmpty()) {
        authWindow->grab().save(screenshotFolder + "/b_auth_welcome.png");
    }

    findButton(authWindow, "Create an account")->click();
    check(counter, "'Create an account' opens the sign-up page", authWindow->currentPage() == AuthWindow::PAGE_SIGN_UP);

    PasswordField *newPassword = authWindow->findChild<PasswordField *>("signUpPassword");
    newPassword->lineEdit()->setText("correcthorse");
    check(counter, "the meter calls a letters-only password weak and gives a tip",
          newPassword->strengthText() == QString::fromUtf8("Strength: Weak · Mix letters with numbers or symbols."));
    newPassword->lineEdit()->setText("Correct Horse 9!");
    check(counter, "the meter calls a long, mixed password very strong",
          newPassword->strengthText() == "Strength: Very strong");

    QPushButton *showButton = newPassword->findChild<QPushButton *>();
    showButton->click();
    bool shown = newPassword->lineEdit()->echoMode() == QLineEdit::Normal && showButton->accessibleName() == "Hide password";
    showButton->click();
    check(counter, "the eye button shows the password and hides it again",
          shown && newPassword->lineEdit()->echoMode() == QLineEdit::Password);

    typeText(authWindow, "signUpName", "Muhammad Saeed");
    typeText(authWindow, "signUpEmail", "saeed@example.com");
    typePassword(authWindow, "signUpConfirm", "Correct Horse 9?");
    findButton(authWindow, "Create account")->click();
    check(counter, "two different passwords are refused before anything is saved",
          shownLabelText(authWindow, "formError") == "The two passwords are not the same." && !auth.hasAnyAccount());
    if (!screenshotFolder.isEmpty()) {
        authWindow->grab().save(screenshotFolder + "/b_auth_sign_up.png");
    }

    QStringList codes = signUpThroughForm(app, authWindow, "Muhammad Saeed", "saeed@example.com", "Correct Horse 9!");
    check(counter, "the account is created and the recovery codes page appears",
          authWindow->currentPage() == AuthWindow::PAGE_CODES && session.isSignedIn());
    check(counter, "10 recovery codes are shown once", codes.size() == 10);
    check(counter, "the password boxes were emptied after use",
          newPassword->lineEdit()->text().isEmpty()
              && authWindow->findChild<PasswordField *>("signUpConfirm")->lineEdit()->text().isEmpty());
    QPushButton *continueButton = findButton(authWindow, "Continue to Finora");
    check(counter, "Continue stays disabled until the codes are marked as saved", !continueButton->isEnabled());
    if (!screenshotFolder.isEmpty()) {
        authWindow->grab().save(screenshotFolder + "/b_auth_recovery_codes.png");
    }
    authWindow->findChild<QCheckBox *>("codesSavedBox")->click();
    check(counter, "ticking 'I have saved these codes' enables Continue", continueButton->isEnabled());

    // The question about the older records appears over the main window.
    lastMessageText.clear();
    QTimer::singleShot(0, acceptClaimQuestion);
    continueButton->click();
    app.processEvents();
    MainWindow *mainWindow = controller.mainWindow();
    check(counter, "after Continue the main window opens and the sign-in window hides",
          mainWindow != nullptr && mainWindow->isVisible() && !authWindow->isVisible());
    check(counter, "Finora asked before moving the record made before accounts",
          lastMessageText.startsWith("This computer has 1 transaction and 0 budgets"));
    if (mainWindow == nullptr) {
        return;
    }
    DashboardPage *dashboard = mainWindow->findChild<DashboardPage *>("dashboardPage");
    check(counter, "the dashboard greets the person by first name",
          shownLabelText(dashboard, "pageTitle").endsWith(", Muhammad"));
    check(counter, "the moved transaction now appears on the dashboard",
          dashboard->findChild<QTableWidget *>("recentTable")->rowCount() == 1);
    check(counter, "the sidebar shows who is signed in", shownLabelText(mainWindow, "accountName") == "Muhammad Saeed");
    check(counter, "a toast welcomes the new person and reports the move",
          toastText(mainWindow).startsWith("Welcome to Finora, Muhammad!")
              && toastText(mainWindow).contains("1 transaction and 0 budgets moved into your account"));
    saveWindowScreenshot(*mainWindow, screenshotFolder, "b_dashboard_signed_in");

    QButtonGroup *navGroup = mainWindow->findChild<QButtonGroup *>("navButtonGroup");
    navGroup->button(PAGE_SETTINGS)->click();
    SettingsPage *settings = mainWindow->findChild<SettingsPage *>("settingsPage");
    check(counter, "Settings shows the profile and the unused recovery codes",
          someLabelContains(settings, "saeed@example.com")
              && someLabelContains(settings, "10 of 10 recovery codes unused"));
    saveWindowScreenshot(*mainWindow, screenshotFolder, "b_settings_account");

    // ---- Sign out, wrong password, sign in again ----
    Sidebar *sidebar = mainWindow->findChild<Sidebar *>();
    findButton(sidebar, "Sign out")->click();
    app.processEvents();
    flushDeletedWidgets();
    check(counter, "signing out closes the main window and shows the sign-in page",
          controller.mainWindow() == nullptr && authWindow->isVisible()
              && authWindow->currentPage() == AuthWindow::PAGE_SIGN_IN && !session.isSignedIn());
    check(counter, "the sign-in page says the person signed out",
          shownLabelText(authWindow, "infoBanner") == "You have signed out.");
    std::vector<Transaction> rows;
    check(counter, "signed out, Finora cannot load any records", !database.loadTransactions(rows));

    signInThroughForm(app, authWindow, "saeed@example.com", "Wrong Horse 1!");
    check(counter, "a wrong password shows only the general message",
          shownLabelText(authWindow, "formError") == SIGN_IN_FAILED_MESSAGE && controller.mainWindow() == nullptr);
    if (!screenshotFolder.isEmpty()) {
        authWindow->grab().save(screenshotFolder + "/b_auth_sign_in_error.png");
    }
    signInThroughForm(app, authWindow, "saeed@example.com", "Correct Horse 9!");
    mainWindow = controller.mainWindow();
    check(counter, "the right password opens the main window with a welcome back",
          mainWindow != nullptr && toastText(mainWindow).startsWith("Welcome back, Muhammad!"));
    if (mainWindow == nullptr) {
        return;
    }
    dashboard = mainWindow->findChild<DashboardPage *>("dashboardPage");
    check(counter, "the same records are there after signing in again",
          dashboard->findChild<QTableWidget *>("recentTable")->rowCount() == 1);

    // ---- Forgot the password: a recovery code ----
    findButton(mainWindow->findChild<Sidebar *>(), "Sign out")->click();
    app.processEvents();
    flushDeletedWidgets();
    findButton(authWindow, "Forgot your password?")->click();
    check(counter, "'Forgot your password?' opens the reset page", authWindow->currentPage() == AuthWindow::PAGE_RESET);
    typeText(authWindow, "resetEmail", "saeed@example.com");
    typeText(authWindow, "resetCode", codes[0]);
    typePassword(authWindow, "resetPassword", "Brand New Pass 42!");
    typePassword(authWindow, "resetConfirm", "Brand New Pass 42!");
    if (!screenshotFolder.isEmpty()) {
        authWindow->grab().save(screenshotFolder + "/b_auth_reset.png");
    }
    findButton(authWindow, "Set new password")->click();
    app.processEvents();
    check(counter, "a recovery code resets the password and says how many codes are left",
          authWindow->currentPage() == AuthWindow::PAGE_SIGN_IN
              && shownLabelText(authWindow, "infoBanner").contains("You have 9 recovery codes left"));
    signInThroughForm(app, authWindow, "saeed@example.com", "Brand New Pass 42!");
    check(counter, "the new password signs in", controller.mainWindow() != nullptr && session.isSignedIn());

    // ---- A second person sees nothing of the first ----
    findButton(controller.mainWindow()->findChild<Sidebar *>(), "Sign out")->click();
    app.processEvents();
    flushDeletedWidgets();
    findButton(authWindow, "Create an account")->click();
    signUpThroughForm(app, authWindow, "Ayesha Khan", "ayesha@example.com", "Sunflower Field 7!");
    authWindow->findChild<QCheckBox *>("codesSavedBox")->click();
    findButton(authWindow, "Continue to Finora")->click();   // no records without an owner are left: no question
    app.processEvents();
    mainWindow = controller.mainWindow();
    check(counter, "a second account opens with its own, empty dashboard",
          mainWindow != nullptr
              && mainWindow->findChild<DashboardPage *>("dashboardPage")->findChild<QTableWidget *>("recentTable")->rowCount() == 0);
    if (mainWindow != nullptr) {
        findButton(mainWindow->findChild<Sidebar *>(), "Sign out")->click();
        app.processEvents();
        flushDeletedWidgets();
    }
}

// Signs out by itself after the idle time: 1 second in this test.
void testIdleSignOut(TestCounter &counter, QApplication &app, const QString &folder)
{
    Session session;
    DatabaseManager database("ui_idle");
    database.open(folder + "/ui_idle.db");
    database.attachSession(session);
    PasswordHasher fast(HASH_INTERACTIVE);
    AuthService auth(database, session, fast, fast);
    std::vector<std::string> codes;
    std::string password = "Correct Horse 9!";
    auth.signUp("Idle Tester", "idle@example.com", password, codes);
    auth.signOut("signed_out");

    AppController controller(database, auth, 1);
    controller.start();
    signInThroughForm(app, controller.authWindow(), "idle@example.com", "Correct Horse 9!");
    check(counter, "the idle test signs in", controller.mainWindow() != nullptr);
    waitFor(3500);
    flushDeletedWidgets();
    check(counter, "after the idle time Finora signs out by itself and says why",
          controller.mainWindow() == nullptr && !session.isSignedIn()
              && shownLabelText(controller.authWindow(), "infoBanner")
                     == "For your security, Finora signed you out after 1 second without activity.");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Finora");
    app.setApplicationVersion(FINORA_VERSION);

    TestCounter counter;
    std::cout << "Finora interface tests\n";
    std::cout << "----------------------\n";

    QString screenshotFolder;
    if (argc == 3 && std::string(argv[1]) == "--screenshots") {
        screenshotFolder = QString::fromLocal8Bit(argv[2]);
        QDir().mkpath(screenshotFolder);
        popupScreenshotFolder = screenshotFolder;
    }

    check(counter, "the bundled Inter and Manrope fonts load", loadBundledFonts());
    check(counter, "light theme and style sheet load", applyLightTheme(app));

    // A real database in a temporary folder that is deleted at the end.
    QTemporaryDir temporaryFolder;
    QString databasePath = temporaryFolder.path() + "/ui_test.db";

    {
        DatabaseManager database("ui_tests");
        check(counter, "the test database opens", database.open(databasePath));

        MainWindow window(database);
        window.resize(1280, 800);
        window.show();

        testNavigation(counter, app, window, screenshotFolder);
        testTransactionsPage(counter, app, window, database, databasePath, screenshotFolder);
    }   // the window, then the database, are destroyed here: like closing Finora

    testDialogValidation(counter);
    testDataAfterRestart(counter, databasePath);

    std::cout << "---- Milestone 4: dashboard, budgets, themes ----\n";
    QString milestone4Path = temporaryFolder.path() + "/ui_test_m4.db";
    testDashboardAndBudgets(counter, app, milestone4Path, screenshotFolder);
    testAppearanceAfterRestart(counter, milestone4Path);

    std::cout << "---- Milestone 4, Phase 4B: sign-in window and accounts ----\n";
    check(counter, "libsodium starts", PasswordHasher::initializeLibrary());
    testAccountsWindow(counter, app, temporaryFolder.path(), screenshotFolder);
    testIdleSignOut(counter, app, temporaryFolder.path());

    std::cout << "----------------------\n";
    std::cout << counter.checksRun << " checks, " << counter.checksFailed << " failed\n";

    if (counter.checksFailed > 0) {
        return 1;
    }
    return 0;
}
