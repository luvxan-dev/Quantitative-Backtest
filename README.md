## Quantitative Backtest CPP
 A backtesting engine that simulates trading a SMA/RSI momentum strategy against
 historical price data. It handles position sizing, stop-losses, and commission costs,
 and reports win rate, profit factor, and max drawdown at the end of each run.
 Wanted to build this from scratch rather than relying on existing backtesting
 frameworks, to really understand the mechanics of how a trading system evaluates
 its own performance
 
## Project Status 
**Core backtesting engine: Complete**

**Live trading integration: In progess**

This project is under active development. The backtesting engine is fully
functional and has been tested against historical OHLCV data. Live trading
via Binance Testnet is the current focus.

## Completed
- CSV data ingestion and parsing (yfinance OHLCV format)
- SMA (Simple Moving Average) and RSI (Relative Strength Index) calculation
- SMA/RSI crossover signal generation
- Full backtesting simulation engine including:
    - Stop-loss logic (auto-exit at 5% drawdown from entry)
    - Position sizing (risk-managed capital allocation per trade)
    - Commission-aware P&L calculations
    - Persistent trade logging to file
- Performance analytics: win rate, profit factor, max drawdown

## In Progress
- Live market data integration via Binance Testnet API (libcurl + nlohmann/jsom)
- Real-time SMA/RSI calculation on live candle data
- Automated live trading loop (Phase 4)

## Planned
- Configurable strategy parameters (via config file or CLI args)
- Additional indicators (MACD, Bollinger Bands)
- Unit tests for indicator calculations
