**⚠️ ÖNEMLİ UYARI: Bu çalışma tamamen eğitim ve siber güvenlik farkındalığı amacıyla hazırlanmıştır. Burada paylaşılan bilgiler ve kodlar, kızılötesi haberleşme sistemlerindeki zafiyetlerin anlaşılması ve bu zafiyetlere karşı savunma mekanizmalarının geliştirilmesi için sunulmuştur. Bu yöntemlerin yetkisiz erişim veya kötü niyetli saldırılar amacıyla kullanılması yasal sorumluluk doğurabilir; tüm sorumluluk uygulayıcıya aittir.
**


## 1. Giriş

Bu çalışma, kızılötesi (Infrared – IR) haberleşme sistemlerinin çalışma prensiplerini, pratik uygulamasını ve bu sistemlerde ortaya çıkabilen **replay (tekrar oynatma)** zafiyetini incelemek amacıyla hazırlanmıştır. Çalışma, hazır kütüphaneler yerine sinyal seviyesine (low-level) odaklanarak gerçekleştirilmiştir. Amaç; IR sinyallerinin fiziksel olarak nasıl üretildiğini, alıcı tarafından nasıl algılandığını ve bu sinyallerin neden kopyalanabilir olduğunu mühendislik bakış açısıyla ortaya koymaktır.

## 2. Kızılötesi İletişim ve Taşıyıcı Frekans

IR haberleşme, ortamdaki gürültüyü (güneş ışığı, aydınlatma) bastırmak için belirli bir taşıyıcı frekans (genellikle 38 kHz) ile modüle edilir. Modülasyon, verinin lojik seviyesini yüksek frekanslı bir taşıyıcıya bindirerek iletilmesidir. Bu sayede alıcı (TSOP), sadece bu frekanstaki sinyalleri işler.

Vishay Semiconductors, "Data Formats for IR Remote Control"– Bu döküman, 38 kHz taşıyıcı frekansının neden standart olduğunu ve gürültü bastırma mekanizmalarını detaylandırır.

### 2.1. Kod Uygulaması: Taşıyıcı Dalga Üretimi

Kod seviyesinde bu modülasyon `mark()` fonksiyonu ile 13 mikrosaniyelik gecikmelerle manuel olarak oluşturulmuştur.

## 3. Veri Kodlama: NEC Protokolü Yaklaşımı

Projedeki zamanlamalar (9000 µs header, 560 µs pulse), endüstri standardı olan **NEC IR Protokolü** ile uyumludur. Başlangıç sinyali (header), alıcının gelen veriye hazırlanmasını sağlar. Veri, darbe uzaklığı (Pulse Distance) yöntemiyle kodlanır; lojik 1 ve 0 arasındaki fark "space" süresidir.

- **Başlangıç Sinyali:** 9000 µs Mark ve 4500 µs Space.
    
- **Lojik 1:** 560 µs Mark + 1680 µs Space.
    
- **Lojik 0:** 560 µs Mark + 560 µs Space.

SB-Projects, "Knowledge: IR Remote Control Theory"– NEC, RC-5 ve Sony gibi protokollerin mikrosaniye düzeyindeki zamanlama standartlarını açıklar.

## 4. Replay (Tekrar Oynatma) Zafiyeti

Replay saldırısı, şifrelenmemiş ve doğrulanmamış statik verilerin yakalanıp tekrar gönderilmesiyle gerçekleştirilir. Statik kodlu sistemlerde her buton basımı aynı bit dizisini üretir. Alıcı taraf, sadece gelen verinin beklenen değere eşit olup olmadığını kontrol eder.

OWASP, "Broken Authentication and Session Management"– Replay saldırılarının oturum yönetimi olmayan sistemlerdeki risklerini ve "Capture-Replay" mekanizmasını tanımlar.

### 4.1. Statik Kumanda Kod Bloğu

Kumanda, her butona basıldığında değişmeyen statik bir veriyi (0xA5) yayınlar.

### 4.2. Statik Alıcı Kod Bloğu

Alıcı tarafı, gelen verinin güncelliğini kontrol etmeden sadece içeriğine bakar.

## 5. Saldırı Mekanizması: ESP32 ReplayRTE

ESP32 üzerinde kullanılan RMT modülü, IR sinyallerini işlemciyi yormadan yüksek hassasiyetle yakalamayı sağlar. RMT, sinyalleri "item" adı verilen ve süre bilgilerini içeren yapılarda saklar. Bu, sinyalin lojik çözümlemesi yapılmasa bile "ham veri" (raw data) olarak kopyalanmasına imkan tanır.

Espressif Systems, "ESP32 Technical Reference Manual"– Donanımsal RMT biriminin mikrosaniye çözünürlüğündeki çalışma prensiplerini belgeler.

### 5.1. Sinyal Yakalama ve Tekrar Oynatma Kod Bloğu

## 6. Savunma Mekanizması: Rolling Code (Kayan Kod)

Rolling Code, her iletimde değişen bir kod yapısı kullanarak yakalanan eski sinyalleri geçersiz kılar. Bu çalışmada, her gönderimde artan bir sayaç mekanizması uygulanmıştır.

Microchip Technology, "AN662: KeeLoq Code Hopping Decoder" – Değişken kod sistemlerinin otomobil anahtarlarında güvenliği nasıl sağladığını anlatan temel dökümandır.

### 6.1. Değişken Kodlu Kumanda Uygulaması

### 6.2. Replay Korumalı Alıcı Uygulaması

Alıcı, gelen sayacı hafızasındaki son sayaçla (`lastAccepted`) kıyaslar ve sadece daha büyük değerleri kabul eder.

## 7. Sonuç

Bu çalışma, IR sistemlerinde güvenliğin sadece veri içeriğine değil, verinin **güncelliğine** bağlı olduğunu kanıtlamıştır. Statik kodlu sistemler ESP32 gibi araçlarla saniyeler içinde kopyalanabilirken, Rolling Code mekanizması kaydedilen sinyali bir sonraki kullanımda geçersiz kılarak güvenliği sağlamaktadır.

# KAYNAKÇA

### 1. Teknik Dokümanlar ve Veri Sayfaları (External References)

- **Vishay Semiconductors**, _"Data Formats for IR Remote Control"_, Rev. 1.3, 2020. (Kızılötesi sinyal modülasyonu ve gürültü bastırma standartları üzerine).
    
- **NEC Corporation**, _"Infrared Transmission Protocol Specification"_, Ver 2.0. (Zamanlama parametreleri ve bit kodlama protokolü üzerine).
    
- **Microchip Technology**, _"AN662: KeeLoq Code Hopping Decoder Implementation"_, 1996. (Rolling Code/Değişken Kod mekanizmasının güvenliği üzerine).
    
- **Espressif Systems**, _"ESP32 Technical Reference Manual - Remote Control Peripheral (RMT) Chapter"_, V4.6. (Donanımsal sinyal yakalama ve oluşturma birimi üzerine).
    
- **OWASP Foundation**, _"Broken Authentication and Session Management: Testing for Replay Attacks"_. (Sinyal kopyalama ve yeniden oynatma saldırılarının teorik analizi üzerine).
    

### 2. Akademik Yayınlar

- **Stallings, W.**, _"Cryptography and Network Security: Principles and Practice"_, Pearson Education, 2017. (Replay saldırılarına karşı 'Nonce' ve 'Counter' (Sayaç) kullanımı üzerine).
    
- **Lathi, B. P., & Ding, Z.**, _"Modern Digital and Analog Communication Systems"_, Oxford University Press, 2009. (Modülasyon ve taşıyıcı frekans temelleri üzerine).
    

### 3. Proje Uygulama Dosyaları (Internal Source Code)

- `kumanda_replay_edilemez.ino`: Rolling code mekanizmasının kumanda tarafındaki uygulaması.
    
- `televizyon_replay_edilemez.ino`: Sayaç kontrolü tabanlı replay bariyerinin alıcı tarafındaki uygulaması.
    
- `televizyon_replay_edilebilir.ino`: Statik kodlu, savunmasız alıcı birimi tasarımı.
    
- `kumanda_replay_edilebilir.ino`: Sabit veri (0xA5) gönderen statik kumanda tasarımı.
    
- `replayRTE.ino`: ESP32 RMT modülü ile geliştirilen sinyal yakalama ve yeniden oynatma (attack) aracı.
