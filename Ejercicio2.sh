clear
rm -f fichero1 >> /dev/null #por si acaso existe, eliminarlo

gcc -o ej1 fuente1.c
gcc -o ej2 fuente2.c
gcc -o ej3 fuente3.c

chmod 777 ej1
chmod 777 ej2
chmod 777 ej3
time ./ej1

rm ej1
rm ej2
rm ej3

