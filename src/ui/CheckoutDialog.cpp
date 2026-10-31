// =====================================================================
//  CheckoutDialog.cpp
// =====================================================================
#include "CheckoutDialog.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "UiHelpers.h"

// What this payment means, in one sentence.
QString checkoutNote(PlanChange change, PlanId plan)
{
    QString name = QString::fromStdString(planName(plan));
    if (change == CHANGE_UPGRADE) {
        return "Your new monthly period starts today. The unused part of your current plan is not refunded.";
    }
    if (change == CHANGE_RENEW) {
        return "This renews " + name + " for one month from today.";
    }
    return "Your monthly period starts today. You can cancel at any time; nothing is ever deleted.";
}

CheckoutDialog::CheckoutDialog(SubscriptionService &sharedService, PlanId chosenPlan, PlanChange change,
                               QWidget *parent)
    : QDialog(parent), service(sharedService), plan(chosenPlan)
{
    setWindowTitle("Checkout");
    setMinimumWidth(540);

    QString name = QString::fromStdString(planName(plan));
    QString price = QString::fromStdString(formatPlanPrice(plan));

    QLabel *heading = new QLabel("Checkout", this);
    heading->setObjectName("dialogTitle");

    QLabel *banner = new QLabel("Simulated payment. Finora collects no card details and no money moves. "
                                "A real version would open the payment provider's own secure page.", this);
    banner->setObjectName("infoBanner");
    banner->setWordWrap(true);

    // The order: plan, price, period and the total for today.
    QDateTime start = QDateTime::currentDateTimeUtc();
    QDateTime end = start.addMonths(BILLING_PERIOD_MONTHS);
    QString period = dateText(start.toString(Qt::ISODate).toStdString()) + QString::fromUtf8(" – ")
                     + dateText(end.toString(Qt::ISODate).toStdString());

    QFrame *order = new QFrame(this);
    order->setObjectName("card");
    QGridLayout *grid = new QGridLayout(order);
    grid->setContentsMargins(18, 14, 18, 14);
    grid->setVerticalSpacing(8);
    QString names[4] = {"Plan", "Price", "Period", "Total today"};
    QString values[4] = {name, price + " per month", period, price + " (simulated)"};
    for (int row = 0; row < 4; row++) {
        QLabel *label = new QLabel(names[row], order);
        label->setObjectName("mutedText");
        QLabel *value = new QLabel(values[row], order);
        value->setObjectName("amountText");
        grid->addWidget(label, row, 0);
        grid->addWidget(value, row, 1, Qt::AlignRight);
    }

    QLabel *note = new QLabel(checkoutNote(change, plan), this);
    note->setObjectName("mutedText");
    note->setWordWrap(true);

    errorLabel = new QLabel(this);
    errorLabel->setObjectName("formError");
    errorLabel->setWordWrap(true);
    errorLabel->hide();

    // Two real actions and a way out. Enter confirms; Escape cancels.
    // Declining is a demonstration of a refused payment, so it is a
    // smaller link on its own row.
    QPushButton *declineButton = new QPushButton("Simulate a declined payment", this);
    declineButton->setObjectName("linkButton");
    declineButton->setCursor(Qt::PointingHandCursor);
    QPushButton *cancelButton = new QPushButton("Cancel", this);
    cancelButton->setObjectName("secondaryButton");
    cancelButton->setCursor(Qt::PointingHandCursor);
    QPushButton *confirmButton = new QPushButton("Confirm simulated payment of " + price, this);
    confirmButton->setObjectName("primaryButton");
    confirmButton->setCursor(Qt::PointingHandCursor);
    confirmButton->setDefault(true);
    connect(declineButton, &QPushButton::clicked, this, &CheckoutDialog::onDeclineClicked);
    connect(cancelButton, &QPushButton::clicked, this, &CheckoutDialog::reject);
    connect(confirmButton, &QPushButton::clicked, this, &CheckoutDialog::onConfirmClicked);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->setSpacing(8);
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(confirmButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(14);
    layout->addWidget(heading);
    layout->addWidget(banner);
    layout->addWidget(order);
    layout->addWidget(note);
    layout->addWidget(errorLabel);
    layout->addWidget(declineButton, 0, Qt::AlignLeft);
    layout->addLayout(buttons);
}

QString CheckoutDialog::paymentReference() const
{
    return reference;
}

bool CheckoutDialog::paymentAttempted() const
{
    return attempted;
}

QString CheckoutDialog::errorText() const
{
    return errorLabel->text();
}

void CheckoutDialog::onConfirmClicked()
{
    pay(false);
}

void CheckoutDialog::onDeclineClicked()
{
    pay(true);
}

// SubscriptionService records the attempt, asks the simulated gateway and
// switches the plan on only if the payment succeeded. A declined payment
// keeps the dialog open with the reason, so the person can try again.
void CheckoutDialog::pay(bool simulateDecline)
{
    attempted = true;
    std::string newReference;
    if (service.payForPlan(plan, simulateDecline, newReference)) {
        reference = QString::fromStdString(newReference);
        accept();
        return;
    }
    errorLabel->setText(service.lastError());
    errorLabel->show();
    adjustSize();   // grow, so the new line does not squeeze the others
}
