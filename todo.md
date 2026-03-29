🏰 ft_irc Proje Yol Haritası (Canvas)

Bu döküman, 42 IRC sunucusu projesi için adım adım uygulama rehberidir.

📊 Kanban Panosu

🔴 Yapılacaklar (To-Do)

🟡 Devam Edenler

🟢 Tamamlananlar

Faz 1: Sınıf Tasarımları (Server, Client)





Faz 2: Soket Kurulumu (initServer)





Faz 3: Poll Döngüsü (Event Loop)





Faz 4: Yeni Bağlantı Yönetimi (Accept)





Faz 5: Veri Okuma ve Buffering





Faz 6: IRC Protokolü & Komutlar (Parser)





🛠️ Teknik Detaylar ve Algoritmalar

1. Sınıf Tasarımları

Neden: Verileri düzenli tutmak için OOP prensiplerini kullanıyoruz.

Server Sınıfı: Sunucu durumunu (port, password), ana soketi ve tüm bağlı istemcilerin listesini (std::map<int, Client>) tutar.

Client Sınıfı: İstemcinin dosya tanımlayıcısını (fd), IP adresini, takma adını (nickname) ve en önemlisi parçalı gelen verileri birleştirmek için bir string buffer'ı tutar.

2. Sunucu Başlatma (Socket Setup)

Algoritma:

socket(): IPv4 (AF_INET) ve TCP (SOCK_STREAM) tipinde bir uç nokta oluştur.

Hata Kontrolü: if (fd < 0) return error;

setsockopt(): SO_REUSEADDR seçeneğini aktif et. Sunucu kapandığında portun "meşgul" kalmasını engeller.

fcntl(): Soketi O_NONBLOCK moduna al. Programın bir okuma sırasında donup kalmasını engeller.

bind(): Soketi belirli bir porta ve IP'ye (genelde 0.0.0.0 yani INADDR_ANY) bağla.

listen(): Soketi dinleme moduna al.

3. Ana Döngü ve I/O Multiplexing (poll)

Neden: Tek bir thread üzerinde yüzlerce kullanıcıyı aynı anda yönetmek için.
Algoritma:

std::vector<struct pollfd> oluştur.

Sunucu soketini (server_fd) bu vektöre POLLIN olayıyla ekle.

Sonsuz döngü başlat:

poll() fonksiyonunu çağır ve bekle.

Döngü ile vektörü gez.

revents & POLLIN kontrolü yap (Veri geldi mi?).

4. Yeni Bağlantı Kabulü (Acceptance)

Algoritma:

Eğer pollfd.fd == server_fd ise:

accept() ile yeni bağlantıyı kabul et (yeni bir client_fd döner).

Yeni client_fd'yi fcntl() ile non-blocking yap.

Yeni bir Client nesnesi oluştur.

Yeni client_fd'yi poll_fds vektörüne ekle ki sonraki döngüde onu da dinleyelim.

5. Veri Okuma ve Tamponlama (The Buffer Logic)

Neden: İnternet paketleri parçalanabilir. "MERHABA\r\n" yerine önce "MER", sonra "HABA\r\n" gelebilir.
Algoritma:

Eğer pollfd.fd bir istemciye aitse:

recv() ile veriyi oku.

Eğer 0 veya -1 dönerse: Bağlantı koptu demektir. close(fd) yap, vektörden ve map'ten sil.

Eğer veri gelirse: Gelen char array'i o Client'ın buffer string'ine ekle (append).

Buffer Kontrolü: while (buffer.find("\r\n") != npos) döngüsü kur.

\r\n'e kadar olan kısmı kes (Bu tam bir komuttur).

Kesilen komutu işle (Parse & Execute).

Kalan kısmı buffer'da bırak.

🛑 Kritik Uyarılar

Sinyal Yönetimi: SIGINT (Ctrl+C) yakalandığında tüm soketleri düzgünce kapatıp belleği temizlediğinden emin ol.

Vektör Yönetimi: poll_fds vektöründen eleman silerken döngü indeksini (i--) güncellemeyi unutma, yoksa bir sonraki elemanı atlarsın.