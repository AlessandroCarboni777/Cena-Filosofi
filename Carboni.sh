#!/bin/bash
clear
gcc -o filosofi Carboni.c
echo "INSERIRE IL NUMERO DI FILOSOFI:   "
read filosofi
echo "INSERIRE IL FLAG DELLO STALLO (negativo/nullo --> disattivato, altrimenti attivato):   "
read stallo
echo "INSERIRE IL FLAG DI SOLUZIONE ALLO STALLO (negativo/nullo --> disattivato, altrimenti attivato):   "
read soluzione
echo "INSERIRE IL FLAG DELLA STARVATION (negativo/nullo --> disattivato, altrimenti attivato):   "
read starvation
sleep 2
clear
echo "ESECUZIONE IN CORSO..."
./filosofi $filosofi $stallo $soluzione $starvation
