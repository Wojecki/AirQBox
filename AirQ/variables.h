// WiFi conf and server
const char* ssid = "Print3d";
const char* password = "QwErTy1+2+3";
const char* serverUrl = "http://192.168.0.101/api/pogoda";

// Time intervals
unsigned long responseTime = 60000;       // Server POST (default 60 s)
unsigned long displayInterval = 60000;   // E-ink refresh (default 60 s)

// Display title
const char* displayTitle = "Retro AirQ Box";