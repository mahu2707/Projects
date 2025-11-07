#include <iostream>
#include <string>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <limits>
#include <cctype>
#include <sstream>
#include <random>
#include <cstdlib>   // llabs

// ========== Forward Decls ==========
class Vehicle;

// ========== Helper Structures ==========
struct PolicyDate {
    int day;
    int month;
    int year;
    PolicyDate(int d=1, int m=1, int y=2024) : day(d), month(m), year(y) {}
};

struct Policy {
    PolicyDate registrationDate;
    PolicyDate renewalDueDate;
    std::string status; // New, Active, Due (Grace), OVERDUE / EXPIRED
    Policy() : registrationDate(), renewalDueDate(), status("New") {}
};

// ========== Date Utilities ==========
time_t dateToTimeT(const PolicyDate& date) {
    std::tm t = std::tm(); // zero-init (C++11)
    t.tm_year = date.year - 1900;
    t.tm_mon  = date.month - 1;
    t.tm_mday = date.day;
    t.tm_hour = 12;     // avoid DST edges
    t.tm_isdst = -1;
    return std::mktime(&t);
}

// (d1 - d2) in days
long long daysBetween(const PolicyDate& d1, const PolicyDate& d2) {
    time_t t1 = dateToTimeT(d1);
    time_t t2 = dateToTimeT(d2);
    double diffSec = std::difftime(t1, t2);
    return static_cast<long long>(std::llround(diffSec / (60.0 * 60.0 * 24.0)));
}

PolicyDate calculateRenewalDate(const PolicyDate& from) {
    PolicyDate renewalDate = from;
    renewalDate.year += 1;
    return renewalDate;
}

std::string dateToString(const PolicyDate& date) {
    std::ostringstream ss;
    ss << std::setw(2) << std::setfill('0') << date.day << "/"
       << std::setw(2) << std::setfill('0') << date.month << "/"
       << date.year;
    return ss.str();
}

int getCurrentSystemYear() {
    std::time_t t = std::time(NULL);
    std::tm* timePtr = std::localtime(&t);
    return timePtr ? (timePtr->tm_year + 1900) : 1970;
}

// ========== Insurance Calculator ==========
class InsuranceCalculator {
private:
    static const double BASE_RATE_PERCENTAGE;   // 0.025
    static const double AGE_DEPRECIATION_RATE;  // 0.03
    static const double MINIMUM_AGE_FACTOR;     // 0.7
    static const double MINIMUM_PREMIUM;        // 5000.0

public:
    static const int    GRACE_PERIOD_DAYS;      // 30

private:
    static const double FINE_PER_DAY;           // 50.0

    double getBaseRate(const Vehicle& vehicle) const;
    double getAgeFactor(int year) const;
    double getFuelTypeAdjustment(const std::string& fuelType) const;

public:
    double calculatePremium(const Vehicle& vehicle) const;
    double calculateLateFine(int daysOverdue) const;
};

const double InsuranceCalculator::BASE_RATE_PERCENTAGE = 0.025;
const double InsuranceCalculator::AGE_DEPRECIATION_RATE = 0.03;
const double InsuranceCalculator::MINIMUM_AGE_FACTOR = 0.7;
const double InsuranceCalculator::MINIMUM_PREMIUM = 5000.0;
const int    InsuranceCalculator::GRACE_PERIOD_DAYS = 30;
const double InsuranceCalculator::FINE_PER_DAY = 50.0;

// ========== Vehicle ==========
class Vehicle {
protected:
    std::string make;
    std::string model;
    int year;
    double originalValue;
    std::string fuelType;
    Policy policy;

public:
    Vehicle(std::string m, std::string mod, int y, double val, std::string ft, PolicyDate regDate)
        : make(m), model(mod), year(y), originalValue(val), fuelType(ft) {
        policy.registrationDate = regDate;
        policy.renewalDueDate   = calculateRenewalDate(regDate);
        policy.status = "New";
    }

    virtual ~Vehicle() {}

    const std::string& getMake()  const { return make; }
    const std::string& getModel() const { return model; }
    int getYear() const { return year; }
    double getOriginalValue() const { return originalValue; }
    std::string getFuelType() const { return fuelType; }
    PolicyDate getRenewalDueDate() const { return policy.renewalDueDate; }
    PolicyDate getRegistrationDate() const { return policy.registrationDate; }
    std::string getPolicyStatus() const { return policy.status; }

    void setPolicyStatus(const std::string& status) { policy.status = status; }
    void setRenewalDueDate(const PolicyDate& d) { policy.renewalDueDate = d; }

    void displayInfo() const {
        std::cout << "\n================================================\n";
        std::cout << "            VEHICLE & POLICY DETAILS\n";
        std::cout << "================================================\n";
        std::cout << std::left << std::setw(22) << "Vehicle:"            << model << " (" << make << ")\n";
        std::cout << std::left << std::setw(22) << "Manufacture Year:"   << year << "\n";
        std::cout << std::left << std::setw(22) << "Fuel Type:"          << fuelType << "\n";
        std::cout << std::left << std::setw(22) << "Original Value:"     << "Rs. " << std::fixed << std::setprecision(2) << originalValue << "\n";
        std::cout << "\n------------------ POLICY DATES ------------------\n";
        std::cout << std::left << std::setw(22) << "Registration Date:"  << dateToString(policy.registrationDate) << "\n";
        std::cout << std::left << std::setw(22) << "Renewal Due Date:"   << dateToString(policy.renewalDueDate)   << "\n";
        std::cout << std::left << std::setw(22) << "Policy Status:"      << policy.status << "\n";
        std::cout << "================================================\n";
    }
};

// ========== Insurance Calculator Impl ==========
double InsuranceCalculator::getBaseRate(const Vehicle& vehicle) const {
    return vehicle.getOriginalValue() * BASE_RATE_PERCENTAGE;
}

double InsuranceCalculator::getAgeFactor(int year) const {
    int currentYear = getCurrentSystemYear();
    int age = std::max(0, currentYear - year);
    double factor = 1.0 - (age * AGE_DEPRECIATION_RATE);
    return std::max(MINIMUM_AGE_FACTOR, factor);
}

double InsuranceCalculator::getFuelTypeAdjustment(const std::string& fuelType) const {
    std::string f = fuelType;
    std::transform(f.begin(), f.end(), f.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (f == "electric") return -500.0;   // discount
    if (f == "diesel")   return  250.0;   // surcharge
    return 0.0;
}

double InsuranceCalculator::calculatePremium(const Vehicle& vehicle) const {
    double baseRate       = getBaseRate(vehicle);
    double ageFactor      = getAgeFactor(vehicle.getYear());
    double fuelAdjustment = getFuelTypeAdjustment(vehicle.getFuelType());
    double premium        = (baseRate * ageFactor) + fuelAdjustment;
    if (premium < MINIMUM_PREMIUM) premium = MINIMUM_PREMIUM;
    return premium;
}

double InsuranceCalculator::calculateLateFine(int daysOverdue) const {
    if (daysOverdue <= GRACE_PERIOD_DAYS) return 0.0;
    int fineDays = daysOverdue - GRACE_PERIOD_DAYS;
    return fineDays * FINE_PER_DAY;
}

// ========== Payment Processor ==========
struct BillBreakdown {
    double basePremium;
    double fine;
    double discount;
    double convenienceFee;
    double emiInterest;
    double gst;
    double total;
    BillBreakdown()
        : basePremium(0.0), fine(0.0), discount(0.0),
          convenienceFee(0.0), emiInterest(0.0), gst(0.0), total(0.0) {}
};

class PaymentProcessor {
public:
    enum Method {
        Card = 1, UPI = 2, NetBanking = 3, Branch = 4, EMI3 = 5, EMI6 = 6
    };

    static const double GST_RATE; // 0.18

    static void printMethods() {
        std::cout << "\n-------------------- PAYMENT METHODS --------------------\n";
        std::cout << "  1. Credit/Debit Card  (1.5% convenience fee, max Rs. 150)\n";
        std::cout << "  2. UPI                (No convenience fee)\n";
        std::cout << "  3. NetBanking         (Rs. 10 flat)\n";
        std::cout << "  4. Pay at Branch      (Rs. 50 handling)\n";
        std::cout << "  5. EMI - 3 months     (12% p.a. simple interest, pro-rated)\n";
        std::cout << "  6. EMI - 6 months     (12% p.a. simple interest, pro-rated)\n";
        std::cout << "---------------------------------------------------------\n";
    }

    static double applyPromo(const std::string& code, double subtotalBeforeGST) {
        std::string c = code;
        std::transform(c.begin(), c.end(), c.begin(),
                       [](unsigned char ch){ return static_cast<char>(std::toupper(ch)); });
        if (c == "LOYAL5") {
            return std::min(500.0, 0.05 * subtotalBeforeGST);
        } else if (c == "FIRST100") {
            return 100.0;
        }
        return 0.0;
    }

    static BillBreakdown quote(Method method, double basePremium, double fine, const std::string& promoCode) {
        BillBreakdown b;
        b.basePremium = basePremium;
        b.fine        = fine;

        // Convenience fee (before GST)
        double conv = 0.0;
        switch (method) {
            case Card:       conv = std::min(150.0, 0.015 * (basePremium + fine)); break;
            case UPI:        conv = 0.0; break;
            case NetBanking: conv = 10.0; break;
            case Branch:     conv = 50.0; break;
            case EMI3:       conv = 0.0; break;
            case EMI6:       conv = 0.0; break;
            default:         conv = 0.0; break;
        }
        b.convenienceFee = conv;

        // EMI interest (simple; p.a. 12% => 0.12 * principal * months/12)
        double emiInt = 0.0;
        int months = 0;
        if (method == EMI3)  months = 3;
        if (method == EMI6)  months = 6;
        if (months > 0) {
            double principal = basePremium + fine; // convenience fee not financed here
            emiInt = 0.12 * principal * (static_cast<double>(months) / 12.0);
        }
        b.emiInterest = emiInt;

        // Promo before GST
        double subtotalBeforeGST = basePremium + fine + conv + emiInt;
        b.discount = applyPromo(promoCode, subtotalBeforeGST);
        subtotalBeforeGST = std::max(0.0, subtotalBeforeGST - b.discount);

        b.gst   = subtotalBeforeGST * GST_RATE;
        b.total = subtotalBeforeGST + b.gst;

        return b;
    }

    static bool confirmAndPay(const BillBreakdown& b, Method /*m*/) {
        std::cout << "\n==================== PAYMENT SUMMARY ====================\n";
        std::cout << std::left << std::setw(28) << "Base Premium:"      << "Rs. " << std::fixed << std::setprecision(2) << b.basePremium << "\n";
        std::cout << std::left << std::setw(28) << "Late Payment Fine:" << "Rs. " << b.fine << "\n";
        std::cout << std::left << std::setw(28) << "Convenience Fee:"   << "Rs. " << b.convenienceFee << "\n";
        std::cout << std::left << std::setw(28) << "EMI Interest:"      << "Rs. " << b.emiInterest << "\n";
        std::cout << std::left << std::setw(28) << "Promo Discount:"    << "Rs. -" << b.discount << "\n";
        std::cout << std::left << std::setw(28) << "GST (18%):"         << "Rs. " << b.gst << "\n";
        std::cout << "----------------------------------------------------------\n";
        std::cout << std::left << std::setw(28) << "TOTAL PAYABLE:"      << "Rs. " << b.total << "\n";
        std::cout << "==========================================================\n";

        std::cout << "Proceed with payment? (y/n): ";
        char ch; std::cin >> ch;
        return (ch == 'y' || ch == 'Y');
    }

    static std::string generateReceiptId() {
        std::time_t now = std::time(NULL);
        std::mt19937_64 rng(static_cast<unsigned long long>(now));
        std::uniform_int_distribution<unsigned long long> dist(100000, 999999);
        std::ostringstream ss;
        ss << "P" << now << "-" << dist(rng);
        return ss.str();
    }

    static const char* methodName(Method m) {
        switch (m) {
            case Card:       return "Credit/Debit Card";
            case UPI:        return "UPI";
            case NetBanking: return "NetBanking";
            case Branch:     return "Pay at Branch";
            case EMI3:       return "EMI (3 months)";
            case EMI6:       return "EMI (6 months)";
            default:         return "Unknown";
        }
    }

    static void printReceipt(const std::string& receiptId,
                             const Vehicle& v,
                             const PolicyDate& oldDue,
                             const PolicyDate& newDue,
                             const BillBreakdown& b,
                             Method m);
};

const double PaymentProcessor::GST_RATE = 0.18;

void PaymentProcessor::printReceipt(const std::string& receiptId,
                                    const Vehicle& v,
                                    const PolicyDate& oldDue,
                                    const PolicyDate& newDue,
                                    const BillBreakdown& b,
                                    Method m) {
    std::cout << "\n==================== TAX INVOICE / RECEIPT ====================\n";
    std::cout << "Receipt ID: " << receiptId << "\n";
    std::cout << "Vehicle   : " << v.getModel() << " (" << v.getMake() << "), " << v.getYear() << "\n";
    std::cout << "Fuel Type : " << v.getFuelType() << "\n";
    std::cout << "Policy    : Due " << dateToString(oldDue) << "  ->  Next Due " << dateToString(newDue) << "\n";
    std::cout << "---------------------------------------------------------------\n";
    std::cout << std::left << std::setw(28) << "Base Premium:"      << "Rs. " << std::fixed << std::setprecision(2) << b.basePremium << "\n";
    std::cout << std::left << std::setw(28) << "Late Payment Fine:" << "Rs. " << b.fine << "\n";
    std::cout << std::left << std::setw(28) << "Convenience Fee:"   << "Rs. " << b.convenienceFee << "\n";
    std::cout << std::left << std::setw(28) << "EMI Interest:"      << "Rs. " << b.emiInterest << "\n";
    std::cout << std::left << std::setw(28) << "Promo Discount:"    << "Rs. -" << b.discount << "\n";
    std::cout << std::left << std::setw(28) << "GST (18%):"         << "Rs. " << b.gst << "\n";
    std::cout << "---------------------------------------------------------------\n";
    std::cout << std::left << std::setw(28) << "TOTAL PAID:"        << "Rs. " << b.total << "\n";
    std::cout << "Payment via: " << methodName(m) << "\n";
    std::cout << "========================== THANK YOU ==========================\n";
}

// ========== Input Helpers ==========
template<typename T>
T getValidInput(const std::string& prompt) {
    T value;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            if (std::cin.peek() == '\n' || std::cin.peek() == EOF) {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return value;
            }
        }
        std::cout << "[!] Invalid input. Please enter a valid value.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

std::string getLineTrimmed(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    std::string::size_type l = s.find_first_not_of(" \t\r\n");
    std::string::size_type r = s.find_last_not_of(" \t\r\n");
    if (l == std::string::npos) return "";
    return s.substr(l, r - l + 1);
}

PolicyDate getValidDate(const std::string& title) {
    PolicyDate date;
    std::cout << title << " (DD MM YYYY): ";
    while (true) {
        if (!(std::cin >> date.day >> date.month >> date.year)) {
            std::cout << "[!] Invalid format. Please enter as DD MM YYYY: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        bool ok = (date.month >= 1 && date.month <= 12) &&
                  (date.day   >= 1 && date.day   <= 31) &&
                  (date.year  >  1900);
        if (ok) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return date;
        }
        std::cout << "[!] Invalid date values. Try again (DD MM YYYY): ";
    }
}

// ========== Policy Status & Renewal Flow ==========
struct StatusResult {
    std::string status;
    long long daysToDueOrOverdue; // +ve => days to due, -ve => overdue days
    double fine;
    bool expired;                 // EXPIRED only when overdue beyond grace
    StatusResult()
        : status("New"), daysToDueOrOverdue(0), fine(0.0), expired(false) {}
};

StatusResult evaluateStatus(const Vehicle& v, const InsuranceCalculator& calc, const PolicyDate& currentDate) {
    StatusResult res;
    long long diff = daysBetween(v.getRenewalDueDate(), currentDate); // due - current
    if (diff >= 0) {
        res.status = "Active";
        res.daysToDueOrOverdue = diff; // days remaining to due
        res.fine = 0.0;
        res.expired = false; // NOT EXPIRED
    } else {
        long long overdueDays = std::llabs(diff);
        if (overdueDays <= InsuranceCalculator::GRACE_PERIOD_DAYS) {
            res.status = "Due (Grace Period)";
            res.daysToDueOrOverdue = -overdueDays;
            res.fine = 0.0;
            res.expired = false; // still NOT EXPIRED
        } else {
            res.status = "OVERDUE / EXPIRED";
            res.daysToDueOrOverdue = -overdueDays;
            res.fine = calc.calculateLateFine(static_cast<int>(overdueDays));
            res.expired = true;  // EXPIRED
        }
    }
    return res;
}

void printStatusBanner(const StatusResult& s) {
    std::cout << "\n==================== POLICY STATUS ====================\n";
    std::cout << "Status    : " << s.status << "\n";
    std::cout << "Coverage  : " << (s.expired ? "EXPIRED" : "NOT EXPIRED") << "\n";
    if (s.daysToDueOrOverdue >= 0) {
        std::cout << "Due in    : " << s.daysToDueOrOverdue << " day(s)\n";
    } else {
        std::cout << "Overdue   : " << -s.daysToDueOrOverdue << " day(s)\n";
    }
    if (s.fine > 0.0) {
        std::cout << "Late fine : Rs. " << std::fixed << std::setprecision(2) << s.fine << "\n";
    }
    std::cout << "======================================================\n";
}

struct BillAndMethod {
    BillBreakdown bill;
    PaymentProcessor::Method method;
    bool valid;
    BillAndMethod()
        : bill(), method(PaymentProcessor::UPI), valid(false) {}
};

BillAndMethod quoteAndChoosePayment(double basePremium, double fine) {
    BillAndMethod out;
    PaymentProcessor::printMethods();
    int methodChoice = getValidInput<int>("Choose a payment method (1-6): ");
    if (methodChoice < 1 || methodChoice > 6) {
        std::cout << "[!] Invalid choice.\n";
        return out;
    }
    out.method = static_cast<PaymentProcessor::Method>(methodChoice);
    std::string promo = getLineTrimmed("Enter promo code (or press Enter to skip): ");
    out.bill = PaymentProcessor::quote(out.method, basePremium, fine, promo);
    out.valid = true;
    return out;
}

void performRenewal(Vehicle& v,
                    const InsuranceCalculator& calc,
                    const PolicyDate& currentDate) {
    // 1) Re-evaluate premium and status/fine
    double basePremium = calc.calculatePremium(v);
    StatusResult s = evaluateStatus(v, calc, currentDate);

    // Guard: renewal allowed ONLY if EXPIRED
    if (!s.expired) {
        std::cout << "\n[INFO] Policy is NOT EXPIRED. Renewal is disabled.\n";
        return;
    }

    // 2) Show details
    v.setPolicyStatus(s.status);
    v.displayInfo();
    printStatusBanner(s);

    // 3) Payment quoting + confirmation
    BillAndMethod qm = quoteAndChoosePayment(basePremium, s.fine);
    if (!qm.valid) {
        std::cout << "Payment selection invalid. Renewal not completed.\n";
        return;
    }
    if (!PaymentProcessor::confirmAndPay(qm.bill, qm.method)) {
        std::cout << "Payment cancelled. Renewal not completed.\n";
        return;
    }

    // 4) Update policy: next due is previous due + 1 year
    PolicyDate oldDue = v.getRenewalDueDate();
    PolicyDate newDue = calculateRenewalDate(oldDue);
    v.setRenewalDueDate(newDue);
    v.setPolicyStatus("Active");

    // 5) Receipt
    std::string receiptId = PaymentProcessor::generateReceiptId();
    PaymentProcessor::printReceipt(receiptId, v, oldDue, newDue, qm.bill, qm.method);
}

// ========== Main ==========
int main() {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "================================================\n";
    std::cout << "     Vehicle Insurance Renewal System v2.0\n";
    std::cout << "================================================\n";

    // Collect inputs
    std::string make, model, fuelType;
    int year;
    double originalValue;
    PolicyDate regDate;

    make  = getLineTrimmed("Enter Vehicle Make : ");
    model = getLineTrimmed("Enter Vehicle Model: ");
    year  = getValidInput<int>("Enter Manufacturing Year (e.g., 2022): ");
    originalValue = getValidInput<double>("Enter Original Vehicle Value (Rs.): ");
    fuelType = getLineTrimmed("Enter Fuel Type (Petrol/Diesel/Electric): ");
    regDate  = getValidDate("Enter Vehicle Registration Date");

    PolicyDate currentDate = getValidDate("Enter Current Date to Check Status");

    Vehicle userVehicle(make, model, year, originalValue, fuelType, regDate);
    InsuranceCalculator calculator;

    // Initial status evaluation
    StatusResult status = evaluateStatus(userVehicle, calculator, currentDate);
    userVehicle.setPolicyStatus(status.status);

    // Summary + premium
    double basePremium = calculator.calculatePremium(userVehicle);
    userVehicle.displayInfo();
    printStatusBanner(status);

    std::cout << "\n==================== RENEWAL SUMMARY ====================\n";
    std::cout << std::left << std::setw(28) << "Base Premium:"      << "Rs. " << basePremium << "\n";
    std::cout << std::left << std::setw(28) << "Late Payment Fine:" << "Rs. " << status.fine << "\n";
    std::cout << "========================================================\n";

    // Renew only if EXPIRED (no prompt otherwise)
    if (status.expired) {
        std::cout << "Policy is EXPIRED. Proceeding to renewal...\n";
        performRenewal(userVehicle, calculator, currentDate);
    } else {
        std::cout << "[INFO] Policy is NOT EXPIRED. Renewal is disabled.\n";
    }

    std::cout << "\nGoodbye!\n";
    return 0;
}

