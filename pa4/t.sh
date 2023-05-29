#!/bin/sh
./reflexer arith.cl | ./refparser | ./refsemant > 1.txt
./mysemant arith.cl > 2.txt
cp 1.txt /media/sf_share/
cp 2.txt /media/sf_share/

./reflexer atoi.cl | ./refparser | ./refsemant > 3.txt
./mysemant atoi.cl > 4.txt
cp 3.txt /media/sf_share/
cp 4.txt /media/sf_share/

./reflexer atoi_test.cl | ./refparser | ./refsemant > 5.txt
./mysemant atoi_test.cl > 6.txt
cp 5.txt /media/sf_share/
cp 6.txt /media/sf_share/

./reflexer book_list.cl | ./refparser | ./refsemant > 7.txt
./mysemant book_list.cl > 8.txt
cp 7.txt /media/sf_share/
cp 8.txt /media/sf_share/

./reflexer cool.cl | ./refparser | ./refsemant > 9.txt
./mysemant cool.cl > 10.txt
cp 9.txt /media/sf_share/
cp 10.txt /media/sf_share/

./reflexer io.cl | ./refparser | ./refsemant > 11.txt
./mysemant io.cl > 12.txt
cp 11.txt /media/sf_share/
cp 12.txt /media/sf_share/

./reflexer hello_world.cl | ./refparser | ./refsemant > 13.txt
./mysemant hello_world.cl > 14.txt
cp 13.txt /media/sf_share/
cp 14.txt /media/sf_share/

./reflexer life.cl | ./refparser | ./refsemant > 15.txt
./mysemant life.cl > 16.txt
cp 15.txt /media/sf_share/
cp 16.txt /media/sf_share/
