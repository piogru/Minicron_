# Minicron_
Projekt - Systemy operacyjne - Linux API

Treść zadania:
Demon uruchamiany w sposób następujący

./minicron <taskfile> <outfile>

Plik taskfile zawiera zadania zapisane w następującym formacie:

\<hour>:\<minutes>:\<command>:\<mode>

Przy czym command jest dowolnym programem.
Parametr mode może mieć jedną z trzech wartości:

    0 - użytkownik chce otrzymać treść, jaką polecenie wypisało na standardowe wyjście (stdout)
    1 - użytkownik chce otrzymać treść, jaką polecenie wypisało na wyjście błędów (stderr).
    2 - użytkownik chce otrzymać treść, jaką polecenie wypisało na standardowe wyjście i wyjście błędów.

Demon wczytuje zadania z pliku i porządkuje je chronologicznie, a następnie zasypia. Budzi się, kiedy nadejdzie czas wykonania pierwszego zadania, tworzy proces potomny wykonujący zadanie, a sam znowu zasypia do czasu kolejnego zadania. Proces potomny wykonuje zadanie uruchamiając nowy proces (lub grupę procesów tworzącą potok w przypadku obsługi potoków). Informacje zlecane przez użytkownika (standardowe wejście/wyjście) są dopisywane do pliku outfile, poprzedzone informacją o poleceniu, które je wygenerowało. Demon kończy pracę po otrzymaniu sygnału SIGINT i ewentualnym zakończeniu bieżącego zadania (zadań). Informacje o uruchomieniu zadania i kodzie wyjścia zakończonego zadania umieszczane są w logu systemowym.

dodatkowo:
    Po otrzymaniu sygnału SIGUSR1 demon ponownie wczytuje zadania z pliku, porzucając ewentualne nie wykonane jeszcze zadania. Po otrzymaniu sygnału SIGUSR2 demon wpisuje do logu systemowego listę zadań, jakie pozostały do wykonania.
    Obsługa potoków, w miejscu  może wystąpić ciąg poleceń odseparowany znakami | (np ls -l | wc -l). W takiej sytuacji standardowe wyjście polecenia przesyłane jest do wejścia kolejnego.

Sposób uruchamiania:

  ./minicron <taskfile> <outfile>

Skrypt testujący:
  
  ./minicron ~/Minicron/taskfile.txt ~/Minicron/outfile.txt

Skrypt zawarty w pliku skrypt.sh powinien zostać umieszczony w katalogu zawierającym program Minicron. Skrypt generuje przykładowy plik taskfile.txt w katalogu „~/Minicron”, następnie uruchamia program.
  
Do uruchomienia programu wymagane jest podanie plików taskfile i outfile. Ze względu na zmianę katalogu bazowego w trakcie działania programu na „/”, pliki w argumentach powinny być podane za pomocą ścieżki. 

Plik taskfile zawiera zadania do wykonania przez program. Wymagane jest istnienie tego pliku w momencie wywołania programu.

Jeśli plik outfile nie istnieje w momencie wywołania programu, zostanie utworzony. Jeśli plik outfile jest już utworzony i zawiera dane, zostaną one usunięte przed pisaniem do pliku.

Struktura zadania w outfile:
  \<hour>:\<minutes>:\<command:\<mode>

  hour:minutes – Godzina i minuta wykonania programu, powinna zostać podana w formacie hh:mm.
  command – Polecenie do wykonania, dopuszczane są potoki. Pojedynczy argument programu nie powinien zawierać w środku spacji np.: „tail -n5”, zamiast „tail -n 5”.
  mode – Informacje, które zostaną wypisane do pliku outfile. Dopuszczalne wartości: {0, 1,2}:
    • 0 – standardowe wyjście,
    • 1 – wyjście błędów,
    • 2 – standardowe wyjście i wyjście błędów.
