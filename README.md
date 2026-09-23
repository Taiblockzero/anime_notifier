# Weekly Anime Notifier 🎌

A C++ program built with Qt 6 that tracks anime episode releases across multiple users and dispatches push notifications via Pushbullet whenever they're ready to watch.

## Features

* **Multi-User Queue:** Sequential, non-blocking task queue processing multiple users and anime watchlists without exceeding Tenrai API rate limits.
* **Pushbullet Integration:** Sends instant push alerts directly to users' devices when a new episode is ready to watch.
* **Daemon-Ready:** Lightweight resource usage optimized to run as a periodic task (e.g., via `cron`) on low-power devices like a Raspberry Pi.

---

## Tech Stack

* **Language:** C++17 / C++20
* **Framework:** Qt 6 (`QCoreApplication`, `QNetworkAccessManager`, `QJsonDocument`)
* **Build System:** `qmake`
* **External APIs:** Jikan API (MyAnimeList REST API) / Pushbullet API

---

## Configuration (`config.json`)

Create a `config.json` file in your execution directory using the following structure:

```json
{
  "users": [
    {
      "username": "Pesho",
      "pushbullet_token": "o.YOUR_PUSHBULLET_TOKEN_HERE",
      "anime_searches": [
        "super no ura",
        "marriagetoxin"
      ]
    },
    {
      "username": "Gosho",
      "pushbullet_token": "o.ANOTHER_PUSHBULLET_TOKEN_HERE",
      "anime_searches": [
        "yomi no tsugai"
      ]
    }
  ]
}
```

---

## Build Instructions

### Prerequisites
* GCC/G++ supporting C++17 or higher
* Qt 6 Development libraries (`qt6-base-dev`)

### Compiling on Linux / Raspberry Pi

1. **Install dependencies:**
   ```bash
   sudo apt update
   sudo apt install -y build-essential qt6-base-dev qt6-base-dev-tools
   ```

2. **Clone the repository:**
   ```bash
   git clone [https://github.com/your-username/anime_notifier.git](https://github.com/your-username/anime_notifier.git)
   cd anime_notifier
   ```

3. **Build using `qmake6`:**
   ```bash
   make clean
   qmake6
   make -j$(nproc)
   ```

4. **Run the executable:**
   ```bash
   ./WeeklyAnimeNotifier
   ```

---

## Automation (Cron Job Setup)

To run the notifier automatically every 15 minutes on your Raspberry Pi:

1. Open your user crontab:
   ```bash
   crontab -e
   ```

2. Add the following entry (adjust paths to match your installation):
   ```bash
   */15 * * * * cd /home/pi/anime_notifier && ./WeeklyAnimeNotifier >> /home/pi/anime_notifier/notifier.log 2>&1
   ```

---

## License

Distributed under the MIT License. See `LICENSE` for more information.
