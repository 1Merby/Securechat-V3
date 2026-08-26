#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// ========== EEPROM SETTINGS ==========
const int EEPROM_SIZE = 512;
const int SSID_ADDR = 0;
const int PASS_ADDR = 64;
const int WEB_USER_ADDR = 128;
const int WEB_PASS_ADDR = 192;
const int CONFIG_FLAG_ADDR = 256;

// ========== AP SETTINGS ==========
String AP_SSID;
String AP_PASSWORD;

// ========== SECURITY SETTINGS ==========
String WEB_USERNAME;
String WEB_PASSWORD;
bool isAuthenticated = false;
bool isConfigured = false;

// ========== WEB SERVER ==========
ESP8266WebServer server(80);

// ========== VALUES ==========
String serialBuffer = "";
String webMessages = "";
const int maxMessages = 50;

// ========== EEPROM FUNCTIONS ==========
bool isFirstBoot() {
  EEPROM.begin(EEPROM_SIZE);
  int flag = EEPROM.read(CONFIG_FLAG_ADDR);
  EEPROM.end();
  return (flag != 0x01);
}

void saveCredentials() {
  EEPROM.begin(EEPROM_SIZE);
  
  // SSID
  int ssidLen = AP_SSID.length();
  if (ssidLen > 32) ssidLen = 32;
  for (int i = 0; i < ssidLen; i++) {
    EEPROM.write(SSID_ADDR + i, AP_SSID[i]);
  }
  EEPROM.write(SSID_ADDR + ssidLen, '\0');
  
  // WiFi password
  int passLen = AP_PASSWORD.length();
  if (passLen > 64) passLen = 64;
  for (int i = 0; i < passLen; i++) {
    EEPROM.write(PASS_ADDR + i, AP_PASSWORD[i]);
  }
  EEPROM.write(PASS_ADDR + passLen, '\0');
  
  // Web username
  int userLen = WEB_USERNAME.length();
  if (userLen > 32) userLen = 32;
  for (int i = 0; i < userLen; i++) {
    EEPROM.write(WEB_USER_ADDR + i, WEB_USERNAME[i]);
  }
  EEPROM.write(WEB_USER_ADDR + userLen, '\0');
  
  // Web password
  int wpassLen = WEB_PASSWORD.length();
  if (wpassLen > 32) wpassLen = 32;
  for (int i = 0; i < wpassLen; i++) {
    EEPROM.write(WEB_PASS_ADDR + i, WEB_PASSWORD[i]);
  }
  EEPROM.write(WEB_PASS_ADDR + wpassLen, '\0');
  
  // Configuration done
  EEPROM.write(CONFIG_FLAG_ADDR, 0x01);
  
  EEPROM.commit();
  EEPROM.end();
  
  isConfigured = true;
  Serial.println("💾 Ayarlar EEPROM'a kaydedildi");
}

void loadCredentials() {
  EEPROM.begin(EEPROM_SIZE);
  
  // Configuration control
  int flag = EEPROM.read(CONFIG_FLAG_ADDR);
  if (flag != 0x01) {
    EEPROM.end();
    isConfigured = false;
    return;
  }
  
  // read SSID
  String ssid = "";
  for (int i = SSID_ADDR; i < SSID_ADDR + 32; i++) {
    char c = char(EEPROM.read(i));
    if (c == '\0') break;
    ssid += c;
  }
  AP_SSID = ssid;
  
  // read Password
  String pass = "";
  for (int i = PASS_ADDR; i < PASS_ADDR + 64; i++) {
    char c = char(EEPROM.read(i));
    if (c == '\0') break;
    pass += c;
  }
  AP_PASSWORD = pass;
  
  // read Web Username
  String user = "";
  for (int i = WEB_USER_ADDR; i < WEB_USER_ADDR + 32; i++) {
    char c = char(EEPROM.read(i));
    if (c == '\0') break;
    user += c;
  }
  WEB_USERNAME = user;
  
  // read Web Password
  String wpass = "";
  for (int i = WEB_PASS_ADDR; i < WEB_PASS_ADDR + 32; i++) {
    char c = char(EEPROM.read(i));
    if (c == '\0') break;
    wpass += c;
  }
  WEB_PASSWORD = wpass;
  
  EEPROM.end();
  isConfigured = true;
  
  Serial.println("✅ EEPROM'dan ayarlar yüklendi");
}

void printCredentials() {
  Serial.println("\n📋 MEVCUT AYARLAR:");
  Serial.println("=============================================");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("WiFi Şifre: ");
  Serial.println(AP_PASSWORD.length() > 0 ? "********" : "(boş)");
  Serial.print("Web Kullanıcı: ");
  Serial.println(WEB_USERNAME);
  Serial.print("Web Şifre: ");
  Serial.println(WEB_PASSWORD.length() > 0 ? "********" : "(boş)");
  Serial.println("=============================================");
}

// ========== FUNCTIONS PROTOTYPE ==========
void addToHistory(String message);
String escapeHTML(String input);
void handleRoot();
void handleLogin();
void handleLogout();
void handleGetSerial();
void handleSendMessage();
void handleGetMessages();
void handleClearSerial();
void handleClearMessages();
bool checkAuth();

// ========== SETUP MENU ==========
void showSetupMenu() {
  Serial.println("\n\n🔐 SECURECHAT - İLK KURULUM");
  Serial.println("=============================================");
  Serial.println("Lütfen aşağıdaki ayarları girin:");
  Serial.println("1. WiFi SSID (ağ adı)");
  Serial.println("2. WiFi Şifre");
  Serial.println("3. Web Kullanıcı Adı");
  Serial.println("4. Web Şifre");
  Serial.println("=============================================");
  Serial.println("\n📝 Komut formatı:");
  Serial.println("  SET_SSID:benim_wifi");
  Serial.println("  SET_PASS:benim_sifre");
  Serial.println("  SET_USER:admin");
  Serial.println("  SET_WPASS:guvenli_sifre");
  Serial.println("  SAVE - ayarları kaydet ve başlat");
  Serial.println("  SHOW - mevcut ayarları göster");
  Serial.println("  RESET - tüm ayarları sıfırla");
  Serial.println("=============================================");
}

void processSetupCommands() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd.startsWith("SET_SSID:")) {
      AP_SSID = cmd.substring(9);
      Serial.print("✅ SSID: ");
      Serial.println(AP_SSID);
    }
    else if (cmd.startsWith("SET_PASS:")) {
      AP_PASSWORD = cmd.substring(9);
      Serial.println("✅ WiFi şifresi ayarlandı");
    }
    else if (cmd.startsWith("SET_USER:")) {
      WEB_USERNAME = cmd.substring(9);
      Serial.print("✅ Web kullanıcı: ");
      Serial.println(WEB_USERNAME);
    }
    else if (cmd.startsWith("SET_WPASS:")) {
      WEB_PASSWORD = cmd.substring(10);
      Serial.println("✅ Web şifresi ayarlandı");
    }
    else if (cmd == "SAVE") {
      if (AP_SSID.length() > 0 && AP_PASSWORD.length() > 0 && 
          WEB_USERNAME.length() > 0 && WEB_PASSWORD.length() > 0) {
        saveCredentials();
        Serial.println("\n✅ Ayarlar kaydedildi! Cihaz yeniden başlatılıyor...");
        delay(2000);
        ESP.restart();
      } else {
        Serial.println("❌ HATA: Tüm alanlar doldurulmalı!");
        Serial.print("   SSID: ");
        Serial.println(AP_SSID.length() > 0 ? "✓" : "✗");
        Serial.print("   WiFi Şifre: ");
        Serial.println(AP_PASSWORD.length() > 0 ? "✓" : "✗");
        Serial.print("   Web Kullanıcı: ");
        Serial.println(WEB_USERNAME.length() > 0 ? "✓" : "✗");
        Serial.print("   Web Şifre: ");
        Serial.println(WEB_PASSWORD.length() > 0 ? "✓" : "✗");
      }
    }
    else if (cmd == "SHOW") {
      Serial.println("\n📋 GİRİLEN AYARLAR:");
      Serial.print("   SSID: ");
      Serial.println(AP_SSID.length() > 0 ? AP_SSID : "(boş)");
      Serial.print("   WiFi Şifre: ");
      Serial.println(AP_PASSWORD.length() > 0 ? "********" : "(boş)");
      Serial.print("   Web Kullanıcı: ");
      Serial.println(WEB_USERNAME.length() > 0 ? WEB_USERNAME : "(boş)");
      Serial.print("   Web Şifre: ");
      Serial.println(WEB_PASSWORD.length() > 0 ? "********" : "(boş)");
    }
    else if (cmd == "RESET") {
      EEPROM.begin(EEPROM_SIZE);
      for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      EEPROM.end();
      Serial.println("🔄 EEPROM temizlendi! Yeniden başlatılıyor...");
      delay(2000);
      ESP.restart();
    }
    else if (cmd.length() > 0) {
      Serial.println("❌ Bilinmeyen komut! Kullanılabilir komutlar:");
      Serial.println("  SET_SSID:<değer>");
      Serial.println("  SET_PASS:<değer>");
      Serial.println("  SET_USER:<değer>");
      Serial.println("  SET_WPASS:<değer>");
      Serial.println("  SAVE");
      Serial.println("  SHOW");
      Serial.println("  RESET");
    }
  }
}

// ========== HTML PAGE (Login) ==========
String getLoginHTML() {
  String html = R"=====(
<!DOCTYPE html><html lang='tr'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>Securechat Giriş</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { 
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  background: #075e54;
  height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
}
.login-container {
  background: white;
  padding: 40px;
  border-radius: 20px;
  box-shadow: 0 20px 60px rgba(0,0,0,0.3);
  max-width: 400px;
  width: 90%;
  text-align: center;
}
.logo {
  font-size: 48px;
  margin-bottom: 20px;
}
h1 {
  color: #075e54;
  margin-bottom: 30px;
  font-size: 24px;
}
.input-group {
  margin-bottom: 20px;
  text-align: left;
}
label {
  display: block;
  margin-bottom: 8px;
  color: #333;
  font-weight: 600;
  font-size: 14px;
}
input {
  width: 100%;
  padding: 12px 16px;
  border: 2px solid #ddd;
  border-radius: 10px;
  font-size: 16px;
  transition: all 0.3s;
}
input:focus {
  border-color: #075e54;
  outline: none;
  box-shadow: 0 0 0 3px rgba(7,94,84,0.1);
}
button {
  width: 100%;
  padding: 14px;
  background: #075e54;
  color: white;
  border: none;
  border-radius: 10px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
  margin-top: 10px;
}
button:hover {
  background: #0b7b6e;
  transform: translateY(-2px);
  box-shadow: 0 5px 15px rgba(7,94,84,0.3);
}
.error {
  color: #f44336;
  margin-top: 15px;
  font-size: 14px;
  padding: 10px;
  background: #ffebee;
  border-radius: 8px;
  display: none;
}
.footer {
  margin-top: 30px;
  color: #999;
  font-size: 12px;
}
</style>
</head>
<body>
<div class='login-container'>
  <div class='logo'>🛡️</div>
  <h1>Securechat Giriş</h1>
  <form id='loginForm' onsubmit='girisYap(event)'>
    <div class='input-group'>
      <label>Kullanıcı Adı</label>
      <input type='text' id='username' placeholder='username' autocomplete='off'>
    </div>
    <div class='input-group'>
      <label>Şifre</label>
      <input type='password' id='password' placeholder='••••••••'>
    </div>
    <button type='submit'>Giriş Yap</button>
    <div id='errorMsg' class='error'>Hatalı kullanıcı adı veya şifre!</div>
  </form>
  <div class='footer'>
    <p>© 2026 Securechat v3.0</p>
  </div>
</div>

<script>
function girisYap(e) {
  e.preventDefault();
  const username = document.getElementById('username').value;
  const password = document.getElementById('password').value;
  
  fetch('/login?username=' + encodeURIComponent(username) + '&password=' + encodeURIComponent(password))
    .then(res => res.text())
    .then(data => {
      if (data === 'OK') {
        window.location.href = '/';
      } else {
        document.getElementById('errorMsg').style.display = 'block';
      }
    });
}
</script>
</body>
</html>
)=====";
  return html;
}

// ========== HTML SAYFASI ==========
String getChatHTML() {
  String html = R"=====(
<!DOCTYPE html><html lang='tr'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>
<title>Securechat</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { 
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  background: #e5ded8;
  height: 100vh;
  display: flex;
  flex-direction: column;
}
.container {
  max-width: 800px;
  width: 100%;
  margin: 0 auto;
  display: flex;
  flex-direction: column;
  height: 100vh;
  background: #f0f2f5;
  position: relative;
}
.header {
  background: #075e54;
  color: white;
  padding: 12px 16px;
  display: flex;
  align-items: center;
  gap: 12px;
  box-shadow: 0 1px 3px rgba(0,0,0,0.2);
}
.header-icon {
  background: rgba(255,255,255,0.2);
  border-radius: 50%;
  width: 40px;
  height: 40px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 20px;
}
.header-info {
  flex: 1;
}
.header-title {
  font-size: 18px;
  font-weight: 600;
  display: flex;
  align-items: center;
  gap: 10px;
}
.header-subtitle {
  font-size: 13px;
  color: rgba(255,255,255,0.9);
  display: flex;
  align-items: center;
  gap: 5px;
}
.status-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background: #4CAF50;
  display: inline-block;
  box-shadow: 0 0 5px #4CAF50;
}
.logout-btn {
  background: rgba(255,255,255,0.2);
  color: white;
  border: none;
  padding: 6px 12px;
  border-radius: 20px;
  font-size: 12px;
  cursor: pointer;
  transition: all 0.2s;
}
.logout-btn:hover {
  background: rgba(255,255,255,0.3);
}
.chat-area {
  flex: 1;
  overflow-y: auto;
  padding: 20px 12px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  background: #efeae2;
  background-image: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100" viewBox="0 0 100 100"><rect width="100" height="100" fill="%23efeae2"/><path d="M0 0 L100 100 M100 0 L0 100" stroke="%23e0d9d1" stroke-width="0.3"/></svg>');
  background-size: 30px 30px;
}
.message {
  display: flex;
  flex-direction: column;
  max-width: 80%;
  margin-bottom: 4px;
  animation: slideIn 0.2s ease;
}
@keyframes slideIn {
  from { opacity: 0; transform: translateY(10px); }
  to { opacity: 1; transform: translateY(0); }
}
.message-left {
  align-self: flex-start;
}
.message-right {
  align-self: flex-end;
}
.message-bubble {
  padding: 8px 12px;
  border-radius: 12px;
  position: relative;
  word-wrap: break-word;
  font-size: 14px;
  line-height: 1.4;
  box-shadow: 0 1px 1px rgba(0,0,0,0.05);
}
.message-left .message-bubble {
  background: white;
  border-top-left-radius: 4px;
  border: 1px solid #e0e0e0;
}
.message-right .message-bubble {
  background: #dcf8c6;
  border-top-right-radius: 4px;
}
.message-info {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 4px;
  margin-top: 2px;
  font-size: 11px;
  color: #667781;
  padding: 0 4px;
}
.message-sender {
  font-weight: 600;
  color: #075e54;
  margin-right: 8px;
  font-size: 12px;
}
.message-time {
  display: flex;
  align-items: center;
}
.system-message {
  align-self: center;
  background: rgba(225, 245, 254, 0.95);
  padding: 6px 16px;
  border-radius: 20px;
  font-size: 12px;
  color: #0288d1;
  margin: 8px 0;
  max-width: 90%;
  border: 1px solid #b3e5fc;
  box-shadow: 0 1px 2px rgba(0,0,0,0.05);
}
.input-area {
  background: #f0f2f5;
  padding: 12px 16px;
  display: flex;
  align-items: center;
  gap: 10px;
  border-top: 1px solid #e0e0e0;
}
.message-input {
  flex: 1;
  padding: 12px 16px;
  border: none;
  border-radius: 24px;
  background: white;
  font-size: 15px;
  outline: none;
  border: 1px solid transparent;
}
.message-input:focus {
  border-color: #075e54;
  box-shadow: 0 0 0 2px rgba(7,94,84,0.1);
}
.send-button {
  width: 48px;
  height: 48px;
  border: none;
  border-radius: 50%;
  background: #075e54;
  color: white;
  font-size: 20px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.2s;
  box-shadow: 0 2px 5px rgba(0,0,0,0.1);
  flex-shrink: 0;
}
.send-button:hover {
  background: #0b7b6e;
  transform: scale(1.05);
}
.date-divider {
  align-self: center;
  background: rgba(225, 245, 254, 0.9);
  padding: 4px 12px;
  border-radius: 12px;
  font-size: 11px;
  color: #5f7d8c;
  margin: 8px 0;
  border: 1px solid #b3e5fc;
}
.chat-area::-webkit-scrollbar {
  width: 6px;
}
.chat-area::-webkit-scrollbar-track {
  background: transparent;
}
.chat-area::-webkit-scrollbar-thumb {
  background: #c0c0c0;
  border-radius: 10px;
}
@media (max-width: 600px) {
  .container { height: 100vh; }
  .message { max-width: 85%; }
}
</style>
</head>
<body>

<div class='container'>
  <div class='header'>
    <div class='header-icon'>🛡️</div>
    <div class='header-info'>
      <div class='header-title'>
        Securechat
        <button class='logout-btn' onclick='cikisYap()'>Çıkış</button>
      </div>
      <div class='header-subtitle'>
        <span class='status-dot'></span>
        <span id='connectionStatus'>Bağlı</span> • Serial ↔ Web
      </div>
    </div>
  </div>
  
  <div class='chat-area' id='chatMessages'>
    <div class='date-divider'>BUGÜN</div>
    <div class='system-message'>
      Sohbet başlatıldı.
    </div>
  </div>
  
  <div class='input-area'>
    <input type='text' class='message-input' id='messageInput' 
           placeholder='Mesaj yazın...' 
           autocomplete='off'>
    <button class='send-button' id='sendBtn' onclick='gonder()'>
      📤
    </button>
  </div>
</div>

<script>
window.onload = () => {
  const chat = document.getElementById('chatMessages');
  const input = document.getElementById('messageInput');
  const status = document.getElementById('connectionStatus');
  
  input.focus();
  
  input.onkeypress = (e) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      gonder();
    }
  };
  
  window.gonder = () => {
    const msg = input.value.trim();
    if (!msg) return;
    
    const now = new Date();
    const saat = now.getHours().toString().padStart(2,'0') + ':' + 
                now.getMinutes().toString().padStart(2,'0');
    
    const mesajDiv = document.createElement('div');
    mesajDiv.className = 'message message-right';
    mesajDiv.innerHTML = `
      <div class='message-bubble'>${kaçis(msg)}</div>
      <div class='message-info'>
        <span class='message-time'>${saat}</span>
      </div>
    `;
    chat.appendChild(mesajDiv);
    chat.scrollTop = chat.scrollHeight;
    
    fetch('/sendMessage?msg=' + encodeURIComponent(msg))
      .catch(err => console.log('Hata:', err));
    
    input.value = '';
    input.focus();
  };
  
  window.cikisYap = () => {
    fetch('/logout').then(() => {
      window.location.href = '/login';
    });
  };
  
  setInterval(() => {
    fetch('/getSerial')
      .then(res => {
        if (!res.ok) window.location.href = '/login';
        return res.text();
      })
      .then(data => {
        if (data && data.trim()) {
          data.split('\n')
            .filter(l => l.trim())
            .forEach(line => {
              const temiz = line.replace('[Serial]', '').trim();
              if (temiz) {
                const now = new Date();
                const saat = now.getHours().toString().padStart(2,'0') + ':' + 
                            now.getMinutes().toString().padStart(2,'0');
                
                const mesajDiv = document.createElement('div');
                mesajDiv.className = 'message message-left';
                mesajDiv.innerHTML = `
                  <div class='message-bubble'>${kaçis(temiz)}</div>
                  <div class='message-info'>
                    <span class='message-sender'>👤 Server</span>
                    <span class='message-time'>${saat}</span>
                  </div>
                `;
                chat.appendChild(mesajDiv);
                chat.scrollTop = chat.scrollHeight;
              }
            });
        }
      })
      .catch(() => {
        status.innerHTML = 'Bağlantı yok';
        document.querySelector('.status-dot').style.background = '#f44336';
      });
  }, 300);
  
  setInterval(() => {
    fetch('/getSerial')
      .then(res => {
        if (res.ok) {
          status.innerHTML = 'Bağlı';
          document.querySelector('.status-dot').style.background = '#4CAF50';
        }
      })
      .catch(() => {
        status.innerHTML = 'Bağlantı yok';
        document.querySelector('.status-dot').style.background = '#f44336';
      });
  }, 2000);
  
  const kaçis = (text) => {
    return text.replace(/&/g, '&amp;')
               .replace(/</g, '&lt;')
               .replace(/>/g, '&gt;')
               .replace(/"/g, '&quot;')
               .replace(/'/g, '&#039;');
  };
};
</script>

</body>
</html>
)=====";
  return html;
}

// ========== SECURITY FUNCTIONS ==========
bool checkAuth() {
  if (!isAuthenticated) {
    server.sendHeader("Location", "/login");
    server.send(302);
    return false;
  }
  return true;
}

void handleLogin() {
  if (server.hasArg("username") && server.hasArg("password")) {
    String user = server.arg("username");
    String pass = server.arg("password");
    String clientIP = server.client().remoteIP().toString();
    
    if (user == WEB_USERNAME && pass == WEB_PASSWORD) {
      isAuthenticated = true;
      server.send(200, "text/plain", "OK");
      Serial.print("✅ [");
      Serial.print(clientIP);
      Serial.println("] Güvenli giriş başarılı");
    } else {
      server.send(200, "text/plain", "FAIL");
      Serial.print("❌ [");
      Serial.print(clientIP);
      Serial.print("] Hatalı giriş denemesi - ");
      Serial.println(user);
    }
  } else {
    server.send(200, "text/html", getLoginHTML());
  }
}

void handleLogout() {
  isAuthenticated = false;
  server.sendHeader("Location", "/login");
  server.send(302);
  Serial.println("🔒 Çıkış yapıldı");
}

void handleRoot() {
  if (!isConfigured) {
    server.send(500, "text/plain", "Cihaz yapılandırılmamış! Lütfen seri port üzerinden kurulum yapın.");
    return;
  }
  
  if (isAuthenticated) {
    server.send(200, "text/html", getChatHTML());
  } else {
    server.sendHeader("Location", "/login");
    server.send(302);
  }
}

// ========== OTHERS ==========
void handleGetSerial() {
  if (!checkAuth()) return;
  server.send(200, "text/plain", serialBuffer);
  serialBuffer = "";
}

void handleSendMessage() {
  if (!checkAuth()) return;
  if (server.hasArg("msg")) {
    String message = server.arg("msg");
    String clientIP = server.client().remoteIP().toString();
    
    Serial.print("[");
    Serial.print(clientIP);
    Serial.print("] ");
    Serial.println(message);
    addToHistory("[" + clientIP + " → Serial] " + message);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Hata");
  }
}

void handleGetMessages() {
  if (!checkAuth()) return;
  server.send(200, "text/html", webMessages);
}

void handleClearSerial() {
  if (!checkAuth()) return;
  serialBuffer = "";
  server.send(200, "text/plain", "Temizlendi");
}

void handleClearMessages() {
  if (!checkAuth()) return;
  webMessages = "";
  server.send(200, "text/plain", "Temizlendi");
}

// ========== TEXT HISTORY ==========
void addToHistory(String message) {
  String timestamp = "[";
  timestamp += String(millis() / 1000);
  timestamp += "s] ";
  
  webMessages = "<div class='message web-message'>" + 
                timestamp + escapeHTML(message) + 
                "</div>" + webMessages;
  
  int messageCount = 0;
  int index = 0;
  while ((index = webMessages.indexOf("<div class='message", index)) != -1) {
    messageCount++;
    index++;
  }
  
  if (messageCount > maxMessages) {
    int lastDiv = webMessages.lastIndexOf("<div class='message");
    if (lastDiv != -1) {
      webMessages = webMessages.substring(0, lastDiv);
    }
  }
}

String escapeHTML(String input) {
  input.replace("&", "&amp;");
  input.replace("<", "&lt;");
  input.replace(">", "&gt;");
  input.replace("\"", "&quot;");
  input.replace("'", "&#39;");
  return input;
}

// ========== SETUP ==========
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n🔐 SECURECHAT");
  Serial.println("=============================================");
  
  // load EEPROM settings 
  loadCredentials();
  
  if (!isConfigured) {
    // First setup mode
    digitalWrite(LED_BUILTIN, LOW);
    
    showSetupMenu();
    
    // Wait until setup is done
    while (!isConfigured) {
      processSetupCommands();
      delay(100);
      
      // LED blink indicates its in setup mode
      static unsigned long lastBlink = 0;
      if (millis() - lastBlink > 500) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        lastBlink = millis();
      }
    }
    
    // Setup done, restarting..
    return;
  }
  
  // Working normal mode
  digitalWrite(LED_BUILTIN, HIGH);
  
  printCredentials();
  
  WiFi.mode(WIFI_AP);
  
  IPAddress localIP(192, 168, 1, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(localIP, gateway, subnet);
  WiFi.softAP(AP_SSID.c_str(), AP_PASSWORD.c_str());
  
  Serial.println("\n📡 WiFi AP başlatıldı:");
  Serial.print("   SSID: ");
  Serial.println(AP_SSID);
  Serial.println("   IP: 192.168.1.1");
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_GET, handleLogin);
  server.on("/logout", HTTP_GET, handleLogout);
  server.on("/getSerial", HTTP_GET, handleGetSerial);
  server.on("/sendMessage", HTTP_GET, handleSendMessage);
  server.on("/getMessages", HTTP_GET, handleGetMessages);
  server.on("/clearSerial", HTTP_GET, handleClearSerial);
  server.on("/clearMessages", HTTP_GET, handleClearMessages);
  
  server.begin();
  
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("\n✅ Securechat başlatıldı!");
  Serial.println("=============================================\n");
  
  serialBuffer = "Securechat sohbet sistemi hazır!\n";
}

// ========== LOOP ==========
void loop() {
  if (!isConfigured) {
    // Setup mode
    processSetupCommands();
    return;
  }
  
  server.handleClient();
  
  if (Serial.available() > 0) {
    String incoming = Serial.readStringUntil('\n');
    incoming.trim();
    
    if (incoming.length() > 0) {
      serialBuffer += "[Serial] " + incoming + "\n";
      
      int lineCount = 0;
      int index = 0;
      while ((index = serialBuffer.indexOf('\n', index)) != -1) {
        lineCount++;
        index++;
      }
      
      if (lineCount > 20) {
        int firstNewline = serialBuffer.indexOf('\n');
        if (firstNewline != -1) {
          serialBuffer = serialBuffer.substring(firstNewline + 1);
        }
      }
      
      addToHistory("[Serial → Web] " + incoming);
    }
  }
}