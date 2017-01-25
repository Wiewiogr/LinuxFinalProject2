./archiwista.out &
sleep 1
./brygadzista.out -ialfa -n3 -t0.1 -m "wiadomosc od alfa"
sleep 2
./brygadzista.out -ibeta -n5 -t0.5 -m "inna wiadomosc od druzyny beta"

wait
