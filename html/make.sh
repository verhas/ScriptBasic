ssplit source.txt
echo 1
./umake
echo 2
sh build.sh
echo 3
rm *.jam
rm *.jim
rm dummy.txt
