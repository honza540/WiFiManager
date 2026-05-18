#include "WiFiManagerWebTemplates.h"

namespace {
const char kSetupPage[] = R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WiFiManager Setup</title>
</head>
<body>
<h1>WiFi Configuration</h1>
<p>Select a network and enter password:</p>
<form action="/save" method="POST">
<label>SSID: <input type="text" name="ssid" size="32"></label><br>
<label>Password: <input type="password" name="pass" size="32"></label><br>
<label>Index (0-2): <input type="number" name="idx" value="0" min="0" max="2"></label><br>
<input type="submit" value="Save WiFi">
</form>
<a href="/scan">Scan Networks</a><br>
<a href="/status">Status</a>
</body>
</html>)";

const char kSavedPage[] = R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WiFiManager Setup Saved</title>
</head>
<body>
<h2>Saved!</h2>
<p>WiFi credentials saved. Connecting...</p>
</body>
</html>)";
}

namespace WiFiManagerWebTemplates {
    String setupPage() {
        return String(kSetupPage);
    }

    String savedPage() {
        return String(kSavedPage);
    }
}
