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
