# Sieci_wisielec
## Opis projektu:

Grać może wiele osób. W momencie dołączenia do serwera gry pierwszej osoby, odliczana jest minuta, po której wszyscy gracze, którzy dołączyli do serwera, rozpoczynają grę, odgadując to samo hasło.
Każdy gracz widzi swojego własnego "wisielca" i niezależnie od innych odgaduje litery. Nałożony jest limit czasowy na podanie kolejnej litery (i/lub odgadnięcie całego hasła). Jeżeli gracz straci wszytskie życia (i/lub nie zdąży odgadnąć hasła), odpada z gry.
Gracze, którzy odgadli hasło, przechodzą do następnego etapu: odgadują nowe hasło, życia się regenerują. Jeżeli do następnego etapu miałby przejść tylko jeden gracz, zostaje on zwycięzcą, a gra się kończy.

Od np. czwartego etapu gracz otrzymuje większą pulę żyć, które jednak przestają się regenerować przy przejściu do następnego etapu.
Program będzie umożliwiał rozgrywanie tylko jednej gry w tym samym czasie, tzn. po odpadnięciu z gry/dołączeniu do serwera po rozpoczęciu gry trzeba będzie poczekać, aż rozpoczęta gra zakończy się, aby móc zagrać. Także kolejne etapy będą rozpoczynać się w tym samym momencie, po zakończeniu poprzedniego etapu przez wszystkich graczy.
Hasła przechowywane będą w banku danych i losowane dla kolejnych gier (etapów). Przewidujemy również możliwość posegregowania haseł na łatwe, średnie i trudne, oraz według kategorii znaczeniowych.

#### TODO:
#### Nie działa dla:
  U1: logowanie
  U1: wyjście
  U1: logowanie
  -- Segmentation fault
