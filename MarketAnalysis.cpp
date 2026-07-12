#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>
#include<string>

using namespace std;

// ================================================================
//  CORE DATA STRUCTURE
// ================================================================

struct Candle {
	string date;
	double open = 0, close = 0, high = 0, low = 0;
	long volume = 0;
};

// ================================================================
//  FUNCTION DECLARATIONS
// ================================================================

void generateSignal(double sma20, double sma50);
double calculateSMA(vector<Candle> candles, int period);
double calculateRSI(const vector<Candle>& candles, int period);
void runBacktest(const vector<Candle>& candles);

// ================================================================
//  CALCULATE SMA — Simple Moving Average
//  Averages the last 'period' closing prices to reveal trend
// ================================================================

double calculateSMA(vector<Candle> candles, int period) {
	if ((int)candles.size() < period) {
		return -1; // not enough data, return sentinel value
	}

	double sum = 0;
	int start = (int)candles.size() - period;

	for (int i = start; i < (int)candles.size(); i++) {
		sum += candles[i].close;
	}

	return sum / period;
}

// ================================================================
//  CALCULATE RSI — Relative Strength Index
//  Measures speed of price changes, range 0-100
//  >70 = overbought, <30 = oversold
// ================================================================

double calculateRSI(const vector<Candle>& candles, int period = 14) {
	if ((int)candles.size() < period + 1) {
		return -1; // not enough data
	}

	double gains = 0, losses = 0;

	int start = (int)candles.size() - period;

	for (int i = start; i < (int)candles.size(); i++) {
		double change = candles[i].close - candles[i - 1].close;
		if (change > 0) gains += change;
		else            losses -= change; // make positive
	}

	double avgGain = gains / period;
	double avgLoss = losses / period;

	if (avgLoss == 0) return 100; // prevent divide by zero

	double rs = avgGain / avgLoss;
	double rsi = 100 - (100 / (1 + rs));
	return rsi;
}

// ================================================================
//  GENERATE SIGNAL — Current market signal based on SMA + RSI
//  Called in main() to show today's recommendation
// ================================================================

void generateSignal(double sma20, double sma50) {
	if (sma20 > sma50) {
		cout << "SIGNAL: Buy - Short-term momentum is above long-term trend" << endl;
	}
	else if (sma20 < sma50) {
		cout << "SIGNAL: Sell - Short-term momentum is below long-term trend" << endl;
	}
	else {
		cout << "SIGNAL: Neutral" << endl;
	}
}

// ================================================================
//  RUN BACKTEST — Simulates the strategy across all historical data
//  Gaps closed: RSI filter, commission costs, position sizing,
//               persistent log file, corrected profit calculation
// ================================================================

void runBacktest(const vector<Candle>& candles) {

	// --- GAP 5: Persistent log file ---
	ofstream logFile("backtest_log.txt");
	if (!logFile.is_open()) {
		cout << "WARNING: Could not open log file. Terminal only." << endl;
	}

	// Helper: print to terminal AND log file simultaneously
	auto log = [&](const string& msg) {
		cout << msg << endl;
		if (logFile.is_open()) logFile << msg << endl;
		};

	// --- GAP 2: Commission per trade ---
	const double COMMISSION = 5.0;

	// --- GAP 3: Position sizing — only risk 20% of cash per trade ---
	const double POSITION_SIZE = 0.20;

	double cash = 10000.0;
	double position = 0.0;
	bool   inTrade = false;
	double entryPrice = 0.0;
	double reservedCash = 0.0; // cash kept aside due to position sizing
	int    wins = 0;
	int    losses = 0;
	int    totalTrades = 0;

	// --- Profit Factor tracking ---
	double totalGross = 0.0; // sum of all winning trade profits
	double totalLoss = 0.0; // sum of all losing trade losses (positive number)

	// --- Max Drawdown tracking ---
	double peakCash = 10000.0; // highest portfolio value seen so far
	double maxDrawdown = 0.0;     // largest peak-to-trough drop in %

	log("\n====== BACKTEST RESULTS ======");

	for (int i = 50; i < (int)candles.size(); i++) {

		// Build a slice of all candles up to today
		vector<Candle> slice(candles.begin(), candles.begin() + i);

		double sma20 = calculateSMA(slice, 20);
		double sma50 = calculateSMA(slice, 50);
		double currentPrice = candles[i].close;

		// --- GAP 1: RSI calculated per slice, used as entry/exit filter ---
		double rsi = calculateRSI(slice, 14);

		// Skip this day if indicators couldn't be calculated
		if (sma20 < 0 || sma50 < 0 || rsi < 0) continue;

		// ---- STOP LOSS CHECK (highest priority) ----
		// Exit immediately if price drops 5% below entry
		if (inTrade && currentPrice < entryPrice * 0.95) {
			inTrade = false;
			double sellValue = (position * currentPrice) - COMMISSION;
			double tradeProfit = sellValue - (position * entryPrice);
			cash = sellValue + reservedCash;
			reservedCash = 0;
			position = 0;
			totalTrades++;
			losses++;
			totalLoss += abs(tradeProfit); // accumulate gross loss

			// Update max drawdown
			if (cash > peakCash) peakCash = cash;
			double drawdown = (peakCash - cash) / peakCash * 100.0;
			if (drawdown > maxDrawdown) maxDrawdown = drawdown;

			log(candles[i].date + " | STOP LOSS at $" + to_string(currentPrice)
				+ " | LOSS: $" + to_string(tradeProfit));
		}

		// ---- BUY CONDITION ----
		// SMA trend is up AND RSI is not overbought (below 70)
		else if (sma20 > sma50 && rsi < 70 && !inTrade) {
			inTrade = true;
			entryPrice = currentPrice;

			// GAP 3: Allocate only 20% of total cash to this trade
			double totalCash = cash;
			double tradeAllocation = totalCash * POSITION_SIZE;
			reservedCash = totalCash - tradeAllocation;

			double afterCommission = tradeAllocation - COMMISSION;
			position = afterCommission / currentPrice;
			cash = 0;

			log(candles[i].date + " | BUY  at $" + to_string(currentPrice)
				+ " | Allocated: $" + to_string(tradeAllocation)
				+ " | Reserved:  $" + to_string(reservedCash)
				+ " | Commission: $" + to_string(COMMISSION));
		}

		// ---- SELL CONDITION ----
		// SMA trend turns down OR RSI extremely overbought (above 80)
		else if ((sma20 < sma50 || rsi > 80) && inTrade) {
			inTrade = false;

			double sellValue = (position * currentPrice) - COMMISSION;
			double tradeProfit = sellValue - (position * entryPrice);
			cash = sellValue + reservedCash;
			reservedCash = 0;
			position = 0;
			totalTrades++;

			if (tradeProfit >= 0) {
				wins++;
				totalGross += tradeProfit; // accumulate gross profit
				log(candles[i].date + " | SELL at $" + to_string(currentPrice)
					+ " | PROFIT: $" + to_string(tradeProfit));
			}
			else {
				losses++;
				totalLoss += abs(tradeProfit); // accumulate gross loss
				log(candles[i].date + " | SELL at $" + to_string(currentPrice)
					+ " | LOSS:   $" + to_string(tradeProfit));
			}

			// Update max drawdown after every closed trade
			if (cash > peakCash) peakCash = cash;
			double drawdown = (peakCash - cash) / peakCash * 100.0;
			if (drawdown > maxDrawdown) maxDrawdown = drawdown;
		}
	}

	// Close any open position at end of data
	if (inTrade) {
		double finalPrice = candles.back().close;
		double sellValue = (position * finalPrice) - COMMISSION;
		cash = sellValue + reservedCash;
		log("Position closed at end of data at $" + to_string(finalPrice));
	}

	// ---- SUMMARY ----
	log("\n==== SUMMARY ====");
	log("Starting capital:   $10,000.00");
	log("Final portfolio:    $" + to_string(cash));
	log("Total return:       $" + to_string(cash - 10000.0));
	log("Return %:           " + to_string(((cash - 10000.0) / 10000.0) * 100) + "%");
	log("Total trades:       " + to_string(totalTrades));
	log("Wins:               " + to_string(wins));
	log("Losses:             " + to_string(losses));
	if (totalTrades > 0) {
		log("Win rate:           " + to_string(((double)wins / totalTrades) * 100) + "%");
	}
	log("Commission paid:    $" + to_string(COMMISSION * totalTrades * 2)); // buy + sell

	// --- Profit Factor ---
	// Gross wins / Gross losses. >1.5 = good, >2.0 = excellent
	if (totalLoss > 0) {
		double profitFactor = totalGross / totalLoss;
		log("Profit Factor:      " + to_string(profitFactor)
			+ "  (>1.5 good | >2.0 excellent)");
	}
	else {
		log("Profit Factor:      N/A (no losing trades)");
	}

	// --- Max Drawdown ---
	// Largest peak-to-trough portfolio drop during the backtest
	// Lower is better. Under 10% is good, under 20% is acceptable
	log("Max Drawdown:       " + to_string(maxDrawdown) + "%"
		+ "  (under 10% = good | under 20% = acceptable)");
	log("=================================");

	if (logFile.is_open()) {
		cout << "\nBacktest log saved to: backtest_log.txt" << endl;
		logFile.close();
	}
}

// ================================================================
//  MAIN — Entry point
//  Loads CSV, runs current analysis, then runs backtest
// ================================================================

int main() {

	// --- Load market data from CSV ---
	ifstream file("bitcoin.csv"); // Input File Stream — connects program to CSV file
	if (!file.is_open()) {
		cout << "ERROR: Could not open gold.csv. Check file path." << endl;
		return 1;
	}

	string line;
	vector<Candle> candles;

	// Skip yfinance's 3 header rows
	getline(file, line);
	getline(file, line);
	getline(file, line);

	while (getline(file, line)) {
		if (line.empty()) continue; // skip blank lines

		stringstream ss(line);
		string token;
		Candle c;

		/**
		 * Read content from "ss"
		 * In ss read everything until the first ',' it sees.
		 * Once ',' is detected store everything before the ',' into token.
		 * Automatically move cursor to the next point for next line extraction.
		 * Convert token to double (tangible numbers) then store in designated slot.
		 * Column order from yfinance: Date, Close, High, Low, Open, Volume
		*/

		getline(ss, c.date, ',');
		getline(ss, token, ','); if (token.empty()) continue; // skip null rows
		c.close = stod(token);
		getline(ss, token, ','); c.high = stod(token);
		getline(ss, token, ','); c.low = stod(token);
		getline(ss, token, ','); c.open = stod(token);
		getline(ss, token, ','); c.volume = stol(token);

		// --- GAP 4: Reject rows with invalid close price ---
		if (c.close <= 0) {
			cout << "WARNING: Skipped invalid row on date: " << c.date << endl;
			continue;
		}

		candles.push_back(c); // append candle to the collection
	}

	cout << "Loaded " << candles.size() << " days of data." << endl;

	// Print all loaded closing prices
	for (auto& c : candles) {
		cout << c.date << " | Close: " << c.close << endl;
	}

	// --- Current day analysis ---
	double sma20 = calculateSMA(candles, 20);
	double sma50 = calculateSMA(candles, 50);
	double rsi = calculateRSI(candles, 14);

	cout << "\n--- CURRENT ANALYSIS ---" << endl;
	cout << "20-day SMA: " << sma20 << endl;
	cout << "50-day SMA: " << sma50 << endl;
	generateSignal(sma20, sma50);

	cout << "RSI (14):   " << rsi << endl;
	if (rsi > 70)      cout << "RSI: Overbought - caution on buying!" << endl;
	else if (rsi < 30) cout << "RSI: Oversold - potential buy opportunity" << endl;
	else               cout << "RSI: Neutral zone." << endl;

	// --- Run backtest across all historical data ---
	runBacktest(candles);

	return 0;
}